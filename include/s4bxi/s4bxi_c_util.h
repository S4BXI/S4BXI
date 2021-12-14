#ifndef S4BXI_C_UTIL_H
#define S4BXI_C_UTIL_H

#include <stdio.h>
#include <stdint.h>

#include "portals4.h"

#ifdef __cplusplus
extern "C" {
#endif

int s4bxi_fprintf(FILE* stream, const char* fmt, ...);
int s4bxi_printf(const char* fmt, ...);
void s4bxi_compute(double flops);
void s4bxi_compute_s(double seconds);
uint32_t s4bxi_get_my_rank();
uint32_t s4bxi_get_my_local_rank();
uint32_t s4bxi_get_rank_number();
/** Number of local peers excluding ourself */
uint32_t s4bxi_num_local_peers();
uint32_t s4bxi_num_nodes();
void s4bxi_local_peers_list(char* list, size_t maxlen);
void s4bxi_get_hostname_from_rank(char *hostname, int rank);
uint32_t s4bxi_global_rank_from_local(uint32_t local);
int s4bxi_get_ptl_process_from_rank(int rank, ptl_process_t* out);
double s4bxi_simtime();
unsigned int s4bxi_is_polling();
void s4bxi_set_polling(unsigned int p);
void s4bxi_barrier();
void s4bxi_set_ptl_process_for_rank(ptl_handle_ni_t ni);
void s4bxi_keyval_store_pointer(char* key, void* value);
void* s4bxi_keyval_fetch_pointer(int rank, char* key);
void s4bxi_set_loglevel(int l);
void s4bxi_use_smpi_implem(int v);

#ifdef __cplusplus
}
#endif

#endif