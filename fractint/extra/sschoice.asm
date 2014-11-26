	.model small
	.stack
	.data
message	db	"Choice: "
l_message	equ $-message
	.code
main	proc	far
	.startup
	mov	bx,0001h
	lea	dx,message
	mov	cx,l_message
	xor	ax,ax
	mov	ah,40h
	int 	21h
	mov	ah,7h
	int 	21h
	cmp	ax,256   ; special key?
	je	special
	sub	ax,304
	jmp	done
special:
	mov	ah,7h
	int 	21h	; eat key
	xor 	ax,ax	
done:
	.exit
main	endp
	end
