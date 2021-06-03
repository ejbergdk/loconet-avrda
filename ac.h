/*
 * ac.h
 *
 * Created: 17-05-2020 13:28:19
 *  Author: Mikael Ejberg Pedersen
 */


#ifndef AC_H_
#define AC_H_

/**
 * Init analog comparator.
 *
 * Call once before interrupts are enabled.
 * NOTE: Handled from hal_ln.c
 */
extern void     ac_init(void);

#endif /* AC_H_ */
