/**
 * Public platform independent Near Field Communication (NFC) library
 *
 * Copyright (C) 2009, Roel Verdult
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 *
 * @file stdbool.h
 * @brief standard boolean functionality
 *
 * Adds support for the beloved "bool" type ;)
 */

#ifndef _STDBOOL_H_
#define	_STDBOOL_H_	

#define	__bool_true_false_are_defined	1

#ifndef __cplusplus

#define	false	0
#define	true	1
typedef	int	bool;

#endif // __cplusplus

#endif // _STDBOOL_H_
