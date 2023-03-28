#include <device_memory_functions.h>
#include <infiniband/mlx5dv.h>
#include <infiniband/verbs.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int print_usage(const char *prog)
{
    printf("\n%s [-d device_name] [-i DM_INIT_VALUE] [-r DM_INCREMENT_ROUNDS] [-s DM_INCREMENT_STEP]\n", prog);
    printf("    DM_INIT_VALUE       >=0\n");
    printf("    DM_INCREMENT_ROUNDS >=0\n");
    printf("    DM_INCREMENT_STEP  >=0\n\n");
    return 0;
}

int process_args(int argc, char *argv[], unsigned long *dm_init_value, unsigned long *dm_increment_rounds, unsigned long *dm_increment_step, char** device_name)
{
    int c;
    char* stopstring;
    
    while ((c = getopt(argc, argv, ":i:r:s:d:")) != -1)
    {
        switch (c)
        {
        case 'd':
			*device_name = strdup(optarg);
			break;
        case 'i':
            *dm_init_value = strtoul(optarg, &stopstring, 10 /*BASE*/);
            if (*dm_init_value < 0)
                goto error;
            break;
        case 'r':
            *dm_increment_rounds = strtoul(optarg, &stopstring, 10 /*BASE*/);
            if (*dm_increment_rounds < 0)
                goto error;
            break;
        case 's':
            *dm_increment_step = strtoul(optarg, &stopstring, 10 /*BASE*/);
            if (*dm_increment_step < 0)
                goto error;
            break;
        default:
        error:
            print_usage(argv[0]);
            return 1;
        }
    }
    return 0;
}

int open_device(struct ibv_context **ctx, char* ib_devname) {
    int num, i;
    struct ibv_device	*ib_dev = NULL;
    struct ibv_device **dev_list = ibv_get_device_list(&num);
    if(dev_list == NULL){
        perror("ibv_get_device_list failed!");
        return 1;
    }
    for (i = 0; i < num; ++i){
        if (!strcmp(ibv_get_device_name(dev_list[i]), ib_devname)){
            ib_dev = dev_list[i];
            break;
        }
    }
    if (!ib_dev) {
        fprintf(stderr, "IB device %s not found\n", ib_devname);
        return 1;
    }
    struct mlx5dv_context_attr attr = {.flags = MLX5DV_CONTEXT_FLAGS_DEVX};
    *ctx = mlx5dv_open_device(dev_list[i], &attr);
    if (*ctx == NULL) {
        printf("Could not open a device!\n");
        return 1;
    }
    printf("%s opened successfully!\n", ibv_get_device_name(dev_list[i]));
    ibv_free_device_list(dev_list);
    return 0;
}

int main(int argc, char *argv[])
{
    unsigned long dm_init_value = 0, dm_increment_rounds = 0, dm_increment_step=0, v = 0, i;
    struct ibv_context *ctx = NULL;
    struct ibv_pd *pd = NULL;
    struct ibv_dm *dm = NULL;
    struct ibv_mr *dm_mr;
    void *memic_atomic_incr_addr = NULL;
    char* device_name = NULL;
    unsigned long curr_value;

    process_args(argc, argv, &dm_init_value, &dm_increment_rounds, &dm_increment_step, &device_name);
    if(open_device(&ctx, device_name) != 0){
        return 1;
    }
    pd = ibv_alloc_pd(ctx);
    if (!pd)
    {
        perror("ibv_alloc_pd failed");
        
    }
    // Since we are going to icrement the buffer using MEMIC atomics and since the PXTH
    // while doing the read operation during the read-modify-write (to implement the atomicity)
    // assumes the data it reads is in Big Endian we set the inital value also in Big Endian.
    // This assumption of the PXTH (that it reads MEMIC in Big Endian in the MEMIC atomic interface) is a HW Bug
    // this BUG will be fixed in CX-8. 
    dm_init_value = htobe64(dm_init_value);
    if (alloc_dm(   ctx, 
                    &dm,
                    sizeof(unsigned long), 
                    &dm_init_value, 
                    pd, 
                    &dm_mr,
                    IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_ATOMIC | IBV_ACCESS_ZERO_BASED,
                    &memic_atomic_incr_addr,
                    DM_MEMIC_ATOMIC_INCREMENT) != 0)
        return 1;

    if (ibv_memcpy_from_dm(&curr_value, dm, 0 /*offset*/, sizeof(unsigned long)) != 0)
    {
        perror("ibv_memcpy_from_dm failed");
    }
    printf("DM initial value: %ld\n", be64toh(curr_value));

    for (i = 0; i < dm_increment_rounds; i++)
    {
        // In CX-7 PXTH assumes the atomic operand it gets from the host is in Big Endian
        // This is a HW BUG and will be fixed in CX-8
        (*(unsigned long *)memic_atomic_incr_addr) = htobe64(dm_increment_step);
        printf("DM_value += %ld ==> ", dm_increment_step);
        usleep(1000 * 500);
        if (ibv_memcpy_from_dm(&curr_value, dm, 0 /*offset*/, sizeof(unsigned long)) != 0)
        {
            perror("ibv_memcpy_from_dm failed");
        }
        printf("DM_value == %ld\n", be64toh(curr_value));
    }

    // Close connection
    if (dealloc_dm(dm, dm_mr) != 0)
        return 1;
    if (ibv_dealloc_pd(pd) != 0)
    {
        perror("ibv_dealloc_pd failed");
        return 1;
    }
    if (ibv_close_device(ctx) != 0)
    {
        perror("ibv_close_device failed");
        return 1;
    }
    return 0;
}