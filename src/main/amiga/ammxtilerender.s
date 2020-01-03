; ammxtilerender.s
;
; (C) 2019 Henryk Richter <henryk.richter@gmx.net>
;
; Render 8x8 tiles using AMMX
;
	machine	ac68080

	XDEF	_HWTiles_render8x8_tile_mask_loresammx
	;XDEF	HWTiles_render8x8_tile_mask_loresammx
	XDEF	_HWTiles_render8x8_tile_mask_loresclipammx

;	xref	ApolloAMMX2On	;byte 0/1

MAX_X	EQU	320	;constants for clipping in lowres mode
MAX_Y	EQU	224	;

;parameter structure
	rsreset
tr_buf		rs.l	1		;uint16_t *buf
tr_tileptr	rs.l	1
tr_x		rs.w	1		;in uint16 units
tr_y		rs.w	1		;in uint16 units
tr_stride	rs.w	1		;in uint16 units
tr_nTilePalette	rs.w	1
tr_nColourDepth	rs.w	1
tr_nMaskColour	rs.w	1
tr_nPaletteOff	rs.w	1

;In: A2 - hwtilerenderarg* args
;
;
_HWTiles_render8x8_tile_mask_loresammx:
;HWTiles_render8x8_tile_mask_loresammx:
	movem.l	d2,-(sp)

	move	tr_x(a2),d2		;uint16 units x
	move	tr_stride(a2),d0	;uint16 units stride
	 ext.l	d2
	lsl.w	#1,d0			;byte units stride*2
	 move	tr_y(a2),d1		;uint16 units y
	muls	d0,d1			;uint16 units y*stride*2
	move.l	tr_buf(a2),a1		;dest
	 add.l	d2,d1			;y*(stride*2)+x
	move.l	tr_tileptr(a2),a0	;src pointer
	 add.l	d2,d1
	add.l	d1,a1			;dest pointer (y*(stride*2)+x)*2

	moveq	#7,d2
	peor	e2,e2,e2		;0
	load	tr_nTilePalette(a2),e5	;off.w x x x
	vperm	#$01010101,e5,e5,e5 ;off.w off.w off.w off.w
	pmul88.w #$800,e5,e5		;off.w<<3 off.w<<3 off.w<<3 off.w<<3
.loop:
	move.l	(a0)+,d1		;current pixel set (8 nibbles)
	beq.s	.skip			;if( d1 ) == maskColor -> skip out if all nibbles are 0
	load	d1,e0			;ab cd ef gh
	lsr.l	#4,d1			;0a bc de fg
	vperm	#$4c5d6e7f,d1,e0,e0     ;0a ab bc cd de ef fg gh
	pand.w	#$0f0f,e0,e0		;()&$f = 0a 0b 0c 0d 0e 0f 0g 0h -> c0 c1 c2 c3 c4 c5 c6 c7
	;
	pcmpeqb	e2,e0,e1		;if( c0 == 0 ) 0xff else 0x00 (C0 C1 C2 C3 C4 C5 C6 C7)
	vperm	#$44556677,e1,e2,e8	;C4 C4 C5 C5 C6 C6 C7 C7
	vperm	#$00112233,e1,e2,e1	;C0 C0 C1 C1 C2 C2 C3 C3
	vperm	#$80818283,e0,e2,e3	;00 0a 00 0b 00 0c 00 0d
	vperm	#$84858687,e0,e2,e4	;00 0e 00 0f 00 0g 00 0h
	paddw	e5,e3,e3
	paddw	e5,e4,e4
	load	(a1),e6
	load	8(a1),e7
	bsel	e6,e1,e3	;(d&!b)|(a&b)
	bsel	e7,e8,e4	;
	store	e3,(a1)
	store	e4,8(a1)
.skip:
	lea	(a1,d0.w),a1
	dbf	d2,.loop

	movem.l	(sp)+,d2
	rts

;In: A2 - hwtilerenderarg* args
;
;same as above but in addition clipped to 0...max_x,0...max_y
;
_HWTiles_render8x8_tile_mask_loresclipammx:
;HWTiles_render8x8_tile_mask_loresclipammx:
	movem.l	d2-d3,-(sp)

	move	tr_x(a2),d2		;uint16 units x
	move	tr_stride(a2),d0	;uint16 units stride
	 ext.l	d2
	lsl.w	#1,d0			;byte units stride*2
	 move	tr_y(a2),d1		;y
	muls	d0,d1			;y*stride*2
	move.l	tr_buf(a2),a1		;dest
	 add.l	d2,d1			;y*(stride*2)+x
	move.l	tr_tileptr(a2),a0	;src pointer
	 add.l	d2,d1			;y*(stride*2)+2*x
	add.l	d1,a1			;dest pointer (y*(stride*2)+x)*2

	peor	e2,e2,e2		;0

	;clipping addition for x positions: just mask out pixels where the 
	;x position points outside, treat as "transparent" pixels
	vperm	#$67676767,d2,d2,d2	;x x x x (.w)
	load	xadd0,e9		;0 1 2 3 (.w)
	load	xadd4,e10		;4 5 6 7 (.w)
	paddw	d2,e9,e9		;x   x+1 x+2 x+3 (.w)
	paddw	d2,e10,e10		;x+4 x+5 x+6 x+7 (.w)
	load.w	#MAX_X-1,e11
	pcmpgtw	e11,e9,e0	;>right edge ? 0xffff : 0x0000
	pcmpgtw e9,e2,e1		;<left edge  ? 0xffff : 0x0000
	por	e0,e1,e3		;(<left edge | >right edge) ? 0xffff : 0x0000

	pcmpgtw	e11,e10,e0	;>right edge ? 0xffff : 0x0000
	pcmpgtw e10,e2,e1		;<left edge  ? 0xffff : 0x0000
	por	e0,e1,e0		;(<left edge | >right edge) ? 0xffff : 0x0000

	vperm	#$02468ace,e3,e0,e9	;(<left edge | >right edge) ? 0xff : 0x00 (.b)
	pand.w	#$0f0f,e9,e9		;(<left edge | >right edge) ? 0x0f : 0x00 (.b)
	peor.w	#$0f0f,e9,e9		;(<left edge | >right edge) ? 0x00 : 0x0f (.b)
					;e9: valid pixels get a mask of 0xf, invalid 0x00

	moveq	#7,d2
	load	tr_nTilePalette(a2),e5	;off.w x x x
	vperm	#$01010101,e5,e5,e5 ;off.w off.w off.w off.w
	pmul88.w #$800,e5,e5		;off.w<<3 off.w<<3 off.w<<3 off.w<<3

	move	tr_y(a2),d3		;y
.loop:
	move.l	(a0)+,d1		;current pixel set (8 nibbles)
	beq.s	.skip			;if( d1 ) == maskColor -> skip out if all nibbles are 0
	cmp.w	#MAX_Y,d3		;<0 or >=MAX_Y ?
	bhs.s	.y_out			;

	load	d1,e0			;ab cd ef gh
	lsr.l	#4,d1			;0a bc de fg
	vperm	#$4c5d6e7f,d1,e0,e0     ;0a ab bc cd de ef fg gh
	pand	e9,e0,e0		;()&$f = 0a 0b 0c 0d 0e 0f 0g 0h -> c0 c1 c2 c3 c4 c5 c6 c7
	;x clipping done within e9
	pcmpeqb	e2,e0,e1		;if( c0 == 0 ) 0xff else 0x00 (C0 C1 C2 C3 C4 C5 C6 C7)
	vperm	#$44556677,e1,e2,e8	;C4 C4 C5 C5 C6 C6 C7 C7
	vperm	#$00112233,e1,e2,e1	;C0 C0 C1 C1 C2 C2 C3 C3
	vperm	#$80818283,e0,e2,e3	;00 0a 00 0b 00 0c 00 0d
	vperm	#$84858687,e0,e2,e4	;00 0e 00 0f 00 0g 00 0h
	paddw	e5,e3,e3
	paddw	e5,e4,e4
	load	(a1),e6
	load	8(a1),e7
	bsel	e6,e1,e3	;(d&!b)|(a&b)
	bsel	e7,e8,e4	;
	store	e3,(a1)
	store	e4,8(a1)
.skip:
	addq	#1,d3
	lea	(a1,d0.w),a1
	dbf	d2,.loop
	bra.s	.end
.y_out:	; either D3 < 0 or D3 > MAX_Y
	tst.w	d3		;<0 ? -> treat this like an empty row
	blt.s	.skip
	;> biggest row: no point in looping anymore, fall through
.end:
	movem.l	(sp)+,d2-d3
	rts
xadd0:	dc.w	0,1,2,3
xadd4:	dc.w	4,5,6,7


