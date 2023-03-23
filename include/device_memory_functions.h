#ifndef DEVICE_MEMORY_FUNCTIONS_H
#define DEVICE_MEMORY_FUNCTIONS_H

#include <infiniband/verbs.h>
#include <infiniband/mlx5dv.h>
#include <stdlib.h>
#include <stdio.h>

#define DM_MEMIC_ATOMIC_INCREMENT (0x0)
#define DM_MEMIC_ATOMIC_TEST_AND_SET (0x1)

int alloc_dm(   struct ibv_context *ctx, 
                struct ibv_dm **dm, 
                int size, 
                void* init_value, 
                struct ibv_pd *pd, 
                struct ibv_mr **mr, 
                uint32_t access_flags, 
                void **bar_addr, 
                int bar_op);

int dealloc_dm(struct ibv_dm *dm, struct ibv_mr *dm_mr);

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
                     uint32_t access_flags);

int unimport_dm(struct ibv_context *imported_ctx,
                struct ibv_pd *imported_pd,
                struct ibv_dm *imported_dm,
                struct ibv_mr *mr);

#endif
