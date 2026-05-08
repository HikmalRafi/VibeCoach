#ifndef TINYML_H
#define TINYML_H

#include "usart.h"
#include "main.h"

#include <stdarg.h>

void ei_printf(const char *format, ...);

#ifdef __cplusplus
extern "C" {
#endif
	void vprint(const char *fmt, va_list argp);
	void add_sensor_data();

#ifdef __cplusplus
}
#endif

#endif
