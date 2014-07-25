/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2013  Dr. JÃ¼rgen Sauermann

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*******

	This file is based on the GNU apl-1.3 package.
	
 *******/

#define char_def( n, _u, _t, _f, _p) AV_ ## n,
#define char_df1(_n, _u, _t, _f, _p)
typedef enum {
   Invalid_CHT = -1,
#include <src/Avec.def>
   MAX_AV,
} CHT_Index;

#define char_def(n, u, t, f, p) UNI_ ## n = u,
#define char_df1(n, u, t, f, p) UNI_ ## n = u,
typedef enum {
#include <src/Avec.def>
  Unicode_0       = 0,            ///< End fo unicode string
  Invalid_Unicode = 0x55AA55AA,   ///< An invalid Unicode.
} Unicode;

typedef gint TokenTag;
typedef gint CharacterFlag;

typedef struct Character_definition
{
   CHT_Index     av_val;      ///< Atomic vector enum of the char.
   Unicode       unicode;     ///< Unicode of the char.
   const char *  char_name;   ///< Name of the char.
   gint          def_line;    ///< Line where the char is defined.
   TokenTag      token_tag;   ///< Token tag for the char.
   CharacterFlag flags;       ///< Character class.
   gint          av_pos;      ///< position in â~N~UAV (== â~N~UAF unicode)
} characters_s;

//   { AV_ ## n, UNI_ ## n, # n, __LINE__, TOK_ ## t, FLG_ ## f, 0x ## p },
#define char_def(n, _u, t, f, p) \
   { AV_ ## n, UNI_ ## n, # n, __LINE__,         0,         0, 0x ## p },
#define char_df1(_n, _u, _t, _f, _p)
characters_s characters[] = {
#include <src/Avec.def>
};
#define char_index(c)    characters[c].av_val
#define char_unicode(c)  characters[c].unicode
#define char_name(c)     characters[c].char_name
#define char_line(c)     characters[c].def_line
#define char_position(c) characters[c].av_pos


