#include <device_memory_functions.h>

int alloc_dm(   struct ibv_context *ctx, 
                struct ibv_dm **dm, 
                int size, 
                void* init_value, 
                struct ibv_pd *pd, 
                struct ibv_mr **mr, 
                uint32_t access_flags, 
                void **bar_addr, 
                int bar_op)
{
    struct ibv_alloc_dm_attr dm_attr = {
        .length = size,
        .log_align_req = 0,
        .comp_mask = 0
    };

    *dm = ibv_alloc_dm(ctx, &dm_attr);
    if (*dm == NULL)
    {
        perror("Error allocating device memory");
        return 1;
    }

    // Init the DM buffer if needed
    if(init_value != NULL)
    {
        if (ibv_memcpy_to_dm(*dm, 0 /*offset*/, init_value, size) != 0)
        {
            perror("ibv_memcpy_to_dm failed");
            return 1;
        }
    }
    
    // Register the DM buffer if needed
    if (mr != NULL)
    {
        *mr = ibv_reg_dm_mr(pd, *dm, 0 /*offset*/, size, access_flags);
        if (*mr == NULL)
        {
            perror("Error registering device memory");
            return 1;
        }
    }

    // Allocate op addr at the bar if needed
    if (bar_addr != NULL)
    {
        *bar_addr = mlx5dv_dm_map_op_addr(*dm, bar_op);
        if (*bar_addr == NULL)
        {
            perror("mlx5dv_dm_map_op_addr failed");
            return 1;
        }
    }
    return 0;
}

int dealloc_dm(struct ibv_dm *dm, struct ibv_mr *mr)
{
    if (mr != NULL)
    {
        if (ibv_dereg_mr(mr) != 0)
        {
            perror("ibv_dereg_mr mr failed");
            return 1;
        }
    }
    if (dm != NULL)
    {
        if (ibv_free_dm(dm) != 0)
        {
            perror("ibv_free_dm failed");
            return 1;
        }
    }
    return 0;
}

int import_dm(int remote_cmd_fd,
                     uint32_t remote_pd_handle,
                     uint32_t remote_dm_handle,
                     struct ibv_context **imported_ctx,
                     struct ibv_pd **imported_pd,
                     struct ibv_dm **imported_dm,
                     int size,
                     struct ibv_mr **mr,
                     void **bar_addr, 
                     int bar_op,
                     uint32_t access_flags)
{
    *imported_ctx = ibv_import_device(remote_cmd_fd);
    if (*imported_ctx == NULL)
    {
        perror("ibv_import_device failed");
        return 1;
    }

    *imported_dm = ibv_import_dm(*imported_ctx, remote_dm_handle);
    if (*imported_dm == NULL)
    {
        perror("ibv_import_dm failed");
        return 1;
    }

    if (imported_pd != NULL && imported_dm != NULL)
    {
        *imported_pd = ibv_import_pd(*imported_ctx, remote_pd_handle);
        if (*imported_pd == NULL)
        {
            perror("ibv_import_pd failed");
            return 1;
        }
    
        *mr = ibv_reg_dm_mr(*imported_pd, *imported_dm, 0 /*offset*/, size, access_flags);
        if (!*mr)
        {
            perror("Error registering device memory");
            return 1;
        }
    }

    if (bar_addr != NULL)
    {
        *bar_addr = mlx5dv_dm_map_op_addr(*imported_dm, bar_op);
        if (*bar_addr == NULL)
        {
            perror("mlx5dv_dm_map_op_addr failed");
            return 1;
        }
    }

    return 0;
}

int unimport_dm(struct ibv_context *imported_ctx,
                struct ibv_pd *imported_pd,
                struct ibv_dm *imported_dm,
                struct ibv_mr *mr)
{
    if (mr != NULL)
    {
        if (ibv_dereg_mr(mr) != 0)
        {
            perror("ibv_dereg_mr mr failed");
            return 1;
        }
    }
    if (imported_dm != NULL)
    {
        ibv_unimport_dm(imported_dm);
    }
    if (imported_pd != NULL)
    {
        ibv_unimport_pd(imported_pd);
    }
    if (imported_ctx != NULL)
    {
        if (ibv_close_device(imported_ctx) != 0)
        {
            perror("ibv_close_device import_ctx failed");
            return 1;
        }
    }
    return 0;
}