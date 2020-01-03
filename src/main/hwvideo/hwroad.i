        ifnd    _INC_HWROAD_I
_INC_HWROAD_I   EQU     1

        rsreset
RD_src0 rs.l    1
RD_src1 rs.l    1
RD_hpos0 rs.l   1
RD_hpos1 rs.l   1
RD_width rs.w   1
RD_dest  rs.l   1
RD_colortable rs.l  1

	rsreset
RDB_pix		rs.l	1	; output pixel data
RDB_ram		rs.l	1	; road RAM
RDB_coloff	rs.w	1	; color offset
RDB_xs		rs.w	1	; width
RDB_ys		rs.w	1	; height
RDB_ctl		rs.b	1	; HWRoad_road_control

        endc    ;_INC_HWROAD_I
