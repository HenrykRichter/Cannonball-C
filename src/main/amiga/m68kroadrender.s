;---------------------------------------------------
; m68kroadrender.s
;
; Road line rendering routines in 68k ASM
;
	machine	ac68080

	XDEF	_m68k_RoadRenderLine0
	XDEF	_m68k_RoadRenderLine1
	XDEF	_m68k_RoadRenderLine2
	XDEF	_m68k_RoadRenderLine3
	XDEF    _m68k_RoadRenderBG


	include	"hwroad.i"

; IN:
;  A0 = struct FrameRoadBG*
;Out:
;  - 
;Notes: A3 could be replaced by A0 in loops
_m68k_RoadRenderBG:
	movem.l	d3-d5/a2-a3,-(sp)

	move.l	RDB_pix(a0),a1	; output pixel data
	move.l	RDB_ram(a0),a2
	move	RDB_ys(a0),d3
	subq	#1,d3
	blt	rrbg_exit
	move	RDB_coloff(a0),d4
	swap	d4
	move	RDB_coloff(a0),d4	;color offset in upper and lower 16 bit
	move	RDB_xs(a0),d5
	lsr	#2,d5
	subq	#1,d5
	blt	rrbg_exit

	move.b	RDB_ctl(a0),d0	;get control byte
	beq	rrbg_mode0
	subq.b	#1,d0
	beq	rrbg_mode1	
	subq.b	#1,d0
	beq	rrbg_mode2

;---------- background mode 3
;mode3:
	lea	$100(a2),a2	;data0->data1
	bra	rrbg_mode0
	
;---------- background mode 2
;same as mode1, just d0 and d1 swapped in moves from a2
rrbg_mode2:
.yloop:
	move	$100(a2),d0	;data1
	move	(a2)+,d1	;data0
	btst	#11,d0		;
	bne.s	.yloopcol0	;if(data1&0x800) color1 ;
	btst	#11,d1
	beq.s	.yloopend	;if(!data0&0x800) continue
	move	d1,d0
.yloopcol0
	move	d0,d1
	swap	d0
	move.l	a1,a3
	move	d1,d0
	and.l	#$7f007f,d0	;
	move	d5,d1
	or.l	d4,d0		;final color
.xloop
	move.l	d0,(a3)+
	move.l	d0,(a3)+
	dbf	d1,.xloop
.yloopend:
	lea	8(a1,d5.w*8),a1	;d5=width/2-1
	dbf	d3,.yloop
	bra	rrbg_exit

;---------- background mode 1		
rrbg_mode1:
.yloop:
	move	$100(a2),d1	;data1
	move	(a2)+,d0	;data0
	btst	#11,d0		;
	bne.s	.yloopcol0	;if(data0&0x800) color0 ;
	btst	#11,d1
	beq.s	.yloopend	;if(!data1&0x800) continue
	move	d1,d0
.yloopcol0
	move	d0,d1
	swap	d0
	move.l	a1,a3
	move	d1,d0
	and.l	#$7f007f,d0	;
	move	d5,d1
	or.l	d4,d0		;final color
.xloop
	move.l	d0,(a3)+
	move.l	d0,(a3)+
	dbf	d1,.xloop
.yloopend:
	lea	8(a1,d5.w*8),a1	;d5=width/2-1
	dbf	d3,.yloop
	bra	rrbg_exit
	
;---------- background mode 0 (and mode 3)
rrbg_mode0:
.yloop:
	move	(a2)+,d0	;data0
	move	d0,d1
	btst	#11,d0		;
	beq.s	.yloopend	;if(!data0&0x800) continue;
	swap	d0
	move.l	a1,a3
	move	d1,d0
	and.l	#$7f007f,d0	;
	move	d5,d1
	or.l	d4,d0		;final color
.xloop
	move.l	d0,(a3)+
	move.l	d0,(a3)+
	dbf	d1,.xloop
.yloopend:
	lea	8(a1,d5.w*8),a1	;d5=width/2-1
	dbf	d3,.yloop
	bra	rrbg_exit

rrbg_exit:
	movem.l	(sp)+,d3-d5/a2-a3
	rts



; IN: 
;  A2 = struct RoadRender*
;OUT:
;  -
_m68k_RoadRenderLine0:
	movem.l	d2-d3/a3,-(sp)

	move.l	RD_src0(a2),a0		;UInt8 *src0
	move.l	RD_colortable(a2),a3
	move.l	RD_dest(a2),a1
	move.l	RD_hpos0(a2),d0

	moveq	#-1,d2
	add	RD_width(a2),d2
	moveq	#0,d3
.loop:
	move.b	(a0,d0.w),d3		;pix0
	cmp.w	#$200,d0
	blt.s	.keep0
	moveq	#3,d3
.keep0:
	move.w	(a3,d3.w*2),(a1)+
	moveq	#0,d3
	addq	#1,d0
	and.w	#$fff,d0
	dbf	d2,.loop

	movem.l	(sp)+,d2-d3/a3
	rts


; IN: 
;  A2 = struct RoadRender*
;OUT:
;  -
_m68k_RoadRenderLine3:
	movem.l	d2-d3/a3,-(sp)

	move.l	RD_src1(a2),a0		;UInt8 *src1
	move.l	RD_colortable(a2),a3
	move.l	RD_dest(a2),a1
	move.l	RD_hpos1(a2),d0

	moveq	#-1,d2
	add	RD_width(a2),d2
	moveq	#0,d3
.loop:
	move.b	(a0,d0.w),d3		;pix1
	cmp.w	#$200,d0
	blt.s	.keep0
	moveq	#3,d3
.keep0:
	move.w	$20(a3,d3.w*2),(a1)+
	moveq	#0,d3
	addq	#1,d0
	and.w	#$fff,d0
	dbf	d2,.loop

	movem.l	(sp)+,d2-d3/a3
	rts


; IN: 
;  A2 = struct RoadRender*
;OUT:
;  -
_m68k_RoadRenderLine1:
	movem.l	d2-d6/a3-a4,-(sp)

	move.l	RD_src0(a2),a0		;UInt8 *src0
	move.l	RD_src1(a2),a1		;UInt8 *src1
	move.l	RD_colortable(a2),a3
	move.l	RD_dest(a2),a4
	move.l	RD_hpos0(a2),d0
	move.l	RD_hpos1(a2),d1

	moveq	#-1,d2
	add	RD_width(a2),d2
	moveq	#0,d3
	moveq	#0,d4
.loop:
	move.b	(a0,d0.w),d3		;pix0
	move.b	(a1,d1.w),d4		;pix1
	cmp.w	#$200,d0
	blt.s	.keep0
	moveq	#3,d3
.keep0:
	cmp.w	#$200,d1
	blt.s	.keep1
	moveq	#3,d4
.keep1:
	move.b	priority_map0(pc,d3.w),d5	;priority_map[0][pix0]
	sub.w	#$10,d3				;set pixel 0 (offset 0*2)
	btst	d4,d5
	beq.s	.setpix0
	move	d4,d3				;set pixel 1 (offset $10*2)
.setpix0:
	move.w	$20(a3,d3.w*2),d5
	moveq	#0,d3
	move	d5,(a4)+
	moveq	#0,d4

	addq	#1,d0
	addq	#1,d1
	and.w	#$fff,d0			;unnecessary ?
	and.w	#$fff,d1			;unnecessary ?
	dbf	d2,.loop

	movem.l	(sp)+,d2-d6/a3-a4
	rts

; IN: 
;  A2 = struct RoadRender*
;OUT:
;  -
;notes: same as _m68k_RoadRenderLine1, just priority_map1
;       instead of priority_map0
_m68k_RoadRenderLine2:
	movem.l	d2-d6/a3-a4,-(sp)

	move.l	RD_src0(a2),a0		;UInt8 *src0
	move.l	RD_src1(a2),a1		;UInt8 *src1
	move.l	RD_colortable(a2),a3
	move.l	RD_dest(a2),a4
	move.l	RD_hpos0(a2),d0
	move.l	RD_hpos1(a2),d1

	moveq	#-1,d2
	add	RD_width(a2),d2
	moveq	#0,d3
	moveq	#0,d4
.loop:
	move.b	(a0,d0.w),d3		;pix0
	move.b	(a1,d1.w),d4		;pix1
	cmp.w	#$200,d0
	blt.s	.keep0
	moveq	#3,d3
.keep0:
	cmp.w	#$200,d1
	blt.s	.keep1
	moveq	#3,d4
.keep1:
	move.b	priority_map1(pc,d3.w),d5	;priority_map[0][pix0]
	sub.w	#$10,d3
	btst	d4,d5
	beq.s	.setpix0
	move	d4,d3				;set pixel 1
.setpix0:
	move.w	$20(a3,d3.w*2),d5
	moveq	#0,d3
	move	d5,(a4)+
	moveq	#0,d4

	addq	#1,d0
	addq	#1,d1
	and.w	#$fff,d0
	and.w	#$fff,d1
	dbf	d2,.loop

	movem.l	(sp)+,d2-d6/a3-a4
	rts


priority_map0:	dc.b	$80,$81,$81,$87,0,0,0,0
priority_map1:	dc.b	$81,$81,$81,$8f,0,0,0,$80
