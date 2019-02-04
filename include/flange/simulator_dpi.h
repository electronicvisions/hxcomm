#pragma once

// clang-format off
extern "C"
{
#include <stdbool.h>
#include <stdint.h>

typedef int dpi_handle_t;

/**
 * @brief Simulator startup wants to initialize the software interface service.
 * @return Handle identifying the service and server instance
 */
dpi_handle_t dpi_comm_init();

/**
 * @brief Simulator wants to cleanly terminate.
 * @note called by simulation before $finish
 */
void dpi_comm_shutdown(dpi_handle_t handle);

/**
 * @brief Simulator provides new data word.
 */
bool dpi_comm_tx(dpi_handle_t handle, uint64_t tx_data);

/**
 * @brief Simulator requests new data word.
 * @return true on new data stored in rx_data/terminate
 */
bool dpi_comm_rx(dpi_handle_t handle, bool rx_ready, bool* terminate, uint64_t* rx_data);

} // extern "C"
// clang-format on
