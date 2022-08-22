/* 
** Copyright 2004, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <malloc.h>
#include <string.h>
#include <phantom_types.h>

char *
ph_strdup(const char *str)
{
	size_t len;
	char *copy;
	
	len = strlen(str) + 1;
	copy = malloc(len);
	if (copy == 0)
		return 0;
	memcpy(copy, str, len);
	return copy;
}


char *ph_strndup(const char *str, size_t n)
{
	size_t len;
	char *copy;
	
	len = strlen(str) + 1;

	n++;
	if( len > n ) len = n;


	copy = malloc(len);
	if (copy == 0)
		return 0;

	memcpy(copy, str, len);
	copy[len-1] = '\0';

	return copy;
}


