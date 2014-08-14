#ifndef KEYMAP_H
#define KEYMAP_H

/***
    http://www.wickensonline.co.uk/apl-unicomp.html
    http://www.fileformat.info/info/unicode/char/2283/index.htm
***/

typedef struct {
  CHT_Index cp_alt;
  CHT_Index cp_shift_alt;
} keymap_s;

keymap_s keymap[] = {
//	    alt		    shift-alt                  key              idx
  {0,		      0},		           //			 0	
  {0,		      0},		   	   //			 1	
  {0,		      0},		   	   //			 2	
  {0,		      0},		   	   //			 3	
  {0,		      0},		   	   //			 4	
  {0,		      0},		   	   //			 5	
  {0,		      0},		   	   //			 6	
  {0,		      0},		   	   //			 7	
  {0,		      0},		   	   //			 8	
  {0,		      0},		   	   //			 9	
  {AV_DIAERESIS,      AV_I_BEAM},		   //	1 		10	
  {AV_OVERBAR,	      AV_DEL_TILDE},		   //	2		11	
  {AV_ASCII_LESS,     AV_SORT_DECENDING},   	   //	3		12	
  {AV_LESS_OR_EQUAL,  AV_SORT_ASCENDING},	   //	4		13	
  {AV_ASCII_EQUAL,    AV_CIRCLE_STILE},		   //	5		14	
  {AV_MORE_OR_EQUAL,  AV_TRANSPOSE},		   //	6		15	
  {AV_ASCII_GREATER,  AV_CIRCLE_BAR},		   //	7		16	
  {AV_NOT_EQUAL,      AV_LOGARITHM},		   //	8		17	
  {AV_OR,	      AV_NOR},		 	   //	9		18	
  {AV_AND,	      AV_NAND},			   //	0		19	
  {AV_MULTIPLY,	      AV_ASCII_EXCLAM},	   	   //	-		20	
  {AV_DIVIDE,	      AV_Quad_DIVIDE},		   //	=		21	
  {0,		      0},		   	   //			22	
  {0,		      0},		   	   //			23
  {AV_ASCII_QUESTION, 0},		   	   //	q		24
  {AV_OMEGA,	      AV_OMEGA_UNDERBAR},	   //	w		25	
  {AV_ELEMENT,	      AV_EPSILON_UBAR},		   //	e		26	
  {AV_RHO,	      0},			   //	r		27	
  {AV_TILDE_OPERATOR, AV_TILDE_DIAERESIS},   	   //	t		28	
  {AV_UP_ARROW,	      AV_YEN},		   	   //	y		29	
  {AV_DOWN_ARROW,     0},		   	   //	u		30	
  {AV_IOTA,	      AV_INDEX_UNDERBAR},	   //	i		31	
  {AV_CIRCLE,	      AV_CIRCLE_DIARESIS},	   //	o		32
  {AV_STAR_OPERATOR,  AV_STAR_DIAERESIS},	   //	p		33
  {AV_LEFT_ARROW,     AV_QUOTE_Quad},	   	   //	[		34	
  {AV_RIGHT_ARROW,    0},			   //	]		35	
  {0,		      0},	   		   //			36	
  {0,		      0},		   	   //			37	
  {AV_ALPHA,	      AV_ALPHA_UNDERBAR},	   //	a		38	
  {AV_LEFT_CEILING,   0},			   //	s		39	
  {AV_LEFT_FLOOR,     AV_DIAMOND},		   //	d		40	
  {AV_ASCII_UNDERSCORE, 0},	 		   //	f		41	
  {AV_NABLA,	      0},			   //	g		42	
  {AV_DELTA,	      AV_DELTA_UNDERBAR},	   //	h		43	
  {AV_RING_OPERATOR,  AV_JOT_DIARESIS},	   	   //	j		44	
  {AV_SINGLE_QUOTE,   0},	   		   //	k		45	
  {AV_Quad_Quad,      AV_SQUISH_Quad},	   	   //	l		46	
  {AV_EXECUTE,        AV_EQUIVALENT},	   	   //	;		47	
  {AV_FORMAT,         AV_NEQUIVALENT},	   	   //	' (fquote},	48	
  {AV_DIAMOND,	      0},			   //	` (backquote},	49	
  {0,		      0},		   	   //	  		50	
  {AV_RIGHT_TACK,      AV_LEFT_TACK},	  	   //	\		51	
  {AV_SUBSET,	      0},			   //	z		52	
  {AV_SUPERSET,	      AV_CHI},		   	   //	x		53	
  //  supposed to be us cent 0x00a2
  // this is a bit of a fake
  {AV_INTERSECTION,   AV_CENT},		   	   //	c		54	
  {AV_UNION,	      0},			   //	v		55	
  {AV_UP_TACK,	      AV_POUND},		   //	b		56	
  {AV_DOWN_TACK,      0},		   	   //	n		57	
  {AV_ASCII_BAR,      0},		  	   //	m		58	
  {AV_COMMENT,        AV_COMMA_BAR}, 	  	   //	, (comma},	59	
  {AV_BACKSLASH_BAR,  AV_DELTA_UNDERBAR},	   //	. (dot},	60
  {AV_FORMAT,	      AV_Quad_COLON},		   // 	/		61
  {0,		      0},		           //			62	
  {0,		      0},		           //			63
};
#define key_alt(k)       keymap[k].cp_alt
#define key_shift_alt(k) keymap[k].cp_shift_alt

/*******

	Don't know if these are interchangeable
	
	EPSILON_UBAR	   0x22F8 F2_FIND
	EPSILON_UNDERBAR   0x2377 F2_FIND

	{AV_DIAMOND,	      0},	\	   //	` (backquote},	49
	is unusable because KDE captures alt-backquote, moved to a-s-d

        {AV_INTERSECTION,   0},		   	   //	c		54
	is supposed to have the cent sign on alt-shift, but gnu apl
	doesn't support that
	
        {AV_RIGHT_ARROW,    0},			   //	]		35
	alt-shift is supposed to have some bizarre symbol, but I can't
	tell what

	{AV_CIRCLE,	      AV_CIRCLE_DIARESIS},  //	o		32
	CIRCLE_DIARESIS not supported under gnu apl
	
        {AV_STAR_OPERATOR,  AV_STAR_DIAERESIS},	   //	p		33	
	STAR_DIARESIS not supported under gnu apl
	
	{AV_FORMAT,	      AV_Quad_COLON},	   // 	/		61
	Quad_COLON not supported under gnu apl
*******/
#endif /* KEYMAP_H */

