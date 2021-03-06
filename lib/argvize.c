/* $Id: argvize.c,v 1.1.1.1 2006/01/05 05:41:16 root Exp $ */

/*
 * Copyright (c) 2000-2002 Opsycon AB  (www.opsycon.se)
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Opsycon AB.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */


#define MAX_AC 100

/*
 *  Take the string pointed by 's', break it up in words and
 *  make a pointer list in 'av'. Return number of 'args'.
 */
int 
argvize(char *av[], char *s)
{
	char **pav = av, c;
	int ac;

	for (ac = 0; ac < MAX_AC; ac++) {
		/* step over cntrls and spaces */
		while (*s && *s <= ' ')
			s++;

		if (!*s)
			break;

		c = *s;
		/* if it's a quote skip forward */
		if (c == '\'' || c == '"') {
			if (pav)
				*pav++ = ++s;
			while (*s && *s != c)
				s++;
			if (*s)
				*s++ = 0;
		} else {		/* find end of word */
			if (pav)
				*pav++ = s;
			while (' ' < *s)
				s++;
		}

		if (*s)
			*s++ = 0;
	}
	return (ac);
}
