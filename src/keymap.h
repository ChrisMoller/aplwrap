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
  {AV_DIAERESIS,      AV_INVERTED_EXCLAM},	   //	1 		10	
  {AV_OVERBAR,	      AV_DIAMOND},		   //	2		11	
  {AV_ASCII_LESS,     0},		   	   //	3		12	
  {AV_LESS_OR_EQUAL,  0},			   //	4		13	
  {AV_ASCII_EQUAL,    0},			   //	5		14	
  {AV_MORE_OR_EQUAL,  0},			   //	6		15	
  {AV_ASCII_GREATER,  0},			   //	7		16	
  {AV_NOT_EQUAL,      AV_Quad_BACKSLASH},	   //	8		17	
  {AV_OR,	      AV_NOR},		 	   //	9		18	
  {AV_AND,	      AV_NAND},			   //	0		19	
  {AV_MULTIPLY,	      AV_EQUIVALENT},	   	   //	-		20	
  {AV_DIVIDE,	      AV_Quad_DIVIDE},		   //	=		21	
  {0,		      0},		   	   //			22	
  {0,		      0},		   	   //			23
#ifdef HAVE_INVERTED_QUESTION	// not used in apl-1.3 Avec.def
  {AV_ASCII_QUESTION, AV_INVERTED_QUESTION},   	   //	q		24
#else
  {0,		      0},		   	   //	q		24
#endif
  {AV_OMEGA,	      AV_CIRCLE_STILE},	 	   //	w		25	
  {AV_ELEMENT,	      AV_EPSILON_UBAR},		   //	e		26	
  {AV_RHO,	      0},			   //	r		27	
  {AV_TILDE_OPERATOR, AV_TRANSPOSE},	   	   //	t		28	
  {AV_UP_ARROW,	      0},		   	   //	y		29	
  {AV_DOWN_ARROW,     0},		   	   //	u		30	
  {AV_IOTA,	      AV_INDEX_UNDERBAR},	   //	i		31	
  {AV_CIRCLE,	      0},			   //	o		32	
  {AV_STAR_OPERATOR,  AV_LOGARITHM},	   	   //	p		33	
  {AV_LEFT_ARROW,     0},		   	   //	[		34	
  {AV_RIGHT_ARROW,    0},			   //	]		35	
  {0,		      0},	   		   //			36	
  {0,		      0},		   	   //			37	
  {AV_ALPHA,	      AV_CIRCLE_BAR},	   	   //	a		38	
  {AV_LEFT_CEILING,   0},			   //	s		39	
  {AV_LEFT_FLOOR,     0},			   //	d		40	
  {AV_ASCII_UNDERSCORE, AV_DEL_TILDE},	 	   //	f		41	
  {AV_NABLA,	      AV_SORT_DECENDING},	   //	g		42	
  {AV_DELTA,	      AV_SORT_ASCENDING},	   //	h		43	
  {AV_RING_OPERATOR,  AV_JOT_DIARESIS},	   	   //	j		44	
  {AV_SINGLE_QUOTE,   AV_Quad_JOT},	   	   //	k		45	
  {AV_Quad_Quad,      AV_QUOTE_Quad},	   	   //	l		46	
  {AV_RIGHT_TACK,     0},		   	   //	;		47	
  {AV_LEFT_TACK,      0},		   	   //	' (fquote},	48	
  {AV_DIAMOND,	      0},			   //	` (backquote},	49	
  {0,		      0},		   	   //	  		50	
  {AV_COMMENT,	      AV_BACKSLASH_BAR},	   //	\		51	
  {AV_SUBSET,	      0},			   //	z		52	
  {AV_SUPERSET,	      0},		   	   //	x		53	
  {AV_INTERSECTION,   AV_COMMENT},		   //	c		54	
  {AV_UNION,	      0},			   //	v		55	
  {AV_UP_TACK,	      AV_EXECUTE},		   //	b		56	
  {AV_DOWN_TACK,      AV_FORMAT},		   //	n		57	
  {AV_ASCII_BAR,      AV_I_BEAM},		   //	m		58	
  {AV_QUOTE_Quad,     AV_COMMA_BAR}, 	  	   //	, (comma},	59	
  {AV_EXECUTE,	      AV_DELTA_UNDERBAR},	   //	. (dot},	60	
  {AV_FORMAT,	      AV_SLASH_BAR},	           // 	/		61	
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
	is unusable because KDE captures alt-backquote, moved to a-s-2

*******/
#endif /* KEYMAP_H */

