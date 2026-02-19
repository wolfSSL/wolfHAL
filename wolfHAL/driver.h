#ifndef WHAL_DRIVER_H
#define WHAL_DRIVER_H

/**
 * @file driver.h
 * @brief Convenience macro for constructing driver function names.
 *
 * WHAL_DRV_FN(DRIVER, OP) pastes to whal_drv_<DRIVER>_<OP>, enforcing the
 * naming convention at every site — declarations, definitions, and call sites.
 */
#define WHAL_DRV_FN(DRIVER, OP) whal_drv_##DRIVER##_##OP

#endif /* WHAL_DRIVER_H */
