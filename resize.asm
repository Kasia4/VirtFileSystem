	section .data
FLOAT_ONE:	dd	1.0
dZEROPNINE:	dq	0.9999
dHALF:		dq	0.5
fHALF:		dd	0.5
FZERO:		dd	0.0
	section .text
	global resize
resize:

					;void resize(uint8_t *inimg, uint8_t *outimg) {
	push	rbx
	push	r10
	push	r11
	push	r12
	push	r13
	push	r14
	push	r15
	push	rbp
	mov	rbp, rsp
					;// *inimg *outimg 16
					;uint8_t *pdy,*pdx,*psi,*psj; 32
					;size_t psStep, pdStep; 16
					;float sx1, sy1, sx2, sy2; 16
					;uint32_t x, y, i, j, destwidth, destheight; 24
					;float destR, destG, destB; 12
					;float fx,fy,fix,fiy,dyf; 20
					;float fxstep, fystep, dxx, dy; 16
					;float AP; 4
					;uint32_t istart,iend,jstart,jend; 16
					;float devX1,devX2,devY1,devY2; 16
	sub	rsp, 92
	%define	inimg	QWORD [rbp-8]
	%define	outimg	QWORD [rbp-16]
	%define	pdy	QWORD [rbp-24]
	%define	pdx	rdi
	%define	psi	rsi
	%define	psj	QWORD [rbp-32]
	%define	psStep	QWORD [rbp-40] ; length of source row
	%define	pdStep	QWORD [rbp-48] ; length of destination row
	%define	sx1	DWORD [rbp-56]
	%define	sx2	DWORD [rbp-60]
	%define	sy1	DWORD [rbp-64]
	%define	sy2	DWORD [rbp-68]
	%define	x	r12d
	%define	y	r13d
	%define	i	r8d
	%define	j	r10d
	%define	destwidth	DWORD [rbp-72]
	%define	destheight	DWORD [rbp-76]
	%define	destR	xmm5
	%define	destG	xmm6
	%define	destB	xmm7
	%define	fx	xmm14
	%define	fy	xmm15
	%define	fix	xmm12
	%define	fiy	DWORD [rbp-80]
	%define	dyf	xmm11
	%define	fxstep	DWORD [rbp-84]
	%define	fystep	DWORD [rbp-88]
	%define	dxx	xmm8
	%define	dy	xmm9
	%define	AP	xmm3
	%define	istart	r14d
	%define	iend	r9d
	%define	jstart	r15d
	%define	jend	r11d
	%define	devX1	DWORD [rbp-92]
	%define	devX2	xmm10
	%define	devY1	DWORD [rbp-96]
	%define	devY2	xmm13
	
	mov	inimg, rdi
	mov	outimg, rsi

					;// width
					;*(uint32_t*)(outimg+0x12) = 200;
	mov	rax, outimg
	mov	DWORD [rax + 0x12], edx	; in rdx is already width
	
					;destwidth = *(uint32_t*)(outimg+0x12) - 1;
	dec	edx
	mov	destwidth, edx
	
					;// height
					;*(uint32_t*)(outimg+0x16) = 200;
	mov	rax, outimg
	mov	DWORD [rax + 0x16], ecx
	
					;destheight = *(uint32_t*)(outimg+0x16) - 1;
	dec	ecx
	mov	destheight, ecx

					;psStep = *(uint32_t*)(inimg+0x12) * 3 ;
	mov	rax, inimg
	mov	eax, DWORD [rax + 0x12]
	mov	edx, eax
	add	rax, rdx
	add	rax, rdx		; source width in bytes

	dec	rax
	and	rax, 0xfffffffffffffffc
	add	rax, 4
	mov	psStep, rax		; source width with padding
	
					;pdStep = *(uint32_t*)(outimg+0x12) *3 ;
	mov	rax, outimg
	mov	eax, DWORD [rax + 0x12] ; destwidth to eax
	mov	edx, eax
	add	rax, rdx
	add	rax, rdx

	dec	rax
	and	rax, 0xfffffffffffffffc
	add	rax, 4
	mov	pdStep, rax		; destination width with padding
    
					;// size of table of output pixels // destHeight * length of destRow
					;*(uint32_t*)(outimg+0x22) = *(uint32_t*)(outimg+0x16) * pdStep;
	mov	rax, outimg
	mov	eax, DWORD [rax + 0x16]
	imul	rax, pdStep
tsize:	
	mov	rcx, outimg
	mov	DWORD [rcx + 0x22], eax
    
					;// bmp size
					;*(uint32_t*)(outimg+0x2) = *(uint32_t*)(outimg+0x22) + 54;
	mov	ecx, eax		; from previously computed value
	add	ecx, 54
	mov	rax, outimg
	mov	DWORD [rax + 0x2], ecx ; size of whole file
    
					;fx = (float)*(uint32_t*)(inimg+0x12) / (float)*(uint32_t*)(outimg+0x12);
	mov	rax, inimg
	mov	eax, DWORD [rax + 0x12] ; source width
	pxor	xmm0, xmm0
	cvtsi2ss	xmm0, rax ; to float
		
	mov	rax, outimg
	mov	ebx, DWORD [rax + 0x12]  ; dest width
	pxor	xmm1, xmm1		
	cvtsi2ss	xmm1, rbx
	divss	xmm0, xmm1
	movss	fx, xmm0 
    
					;fy = (float)*(uint32_t*)(inimg+0x16) / (float)*(uint32_t*)(outimg+0x16);
	mov	rax, inimg
	mov	eax, DWORD [rax + 0x16]
	pxor	xmm0, xmm0
	cvtsi2ss	xmm0, rax
	mov	rax, outimg
	mov	ebx, DWORD [rax + 0x16]
	pxor	xmm1, xmm1
	cvtsi2ss	xmm1, rbx
	divss	xmm0, xmm1
	movss	fy, xmm0
	
	
					;fix = 1/fx;
	pxor	xmm0, xmm0
	movss	xmm0, [FLOAT_ONE]
	divss	xmm0, fx
	movss	fix, xmm0


					;fiy = 1/fy;
	pxor	xmm0, xmm0
	movss	xmm0, [FLOAT_ONE]
	divss	xmm0, fy
	movss	fiy, xmm0
	
					;fxstep = 0.9999 * fx;
	cvtss2sd	xmm0, fx
	movsd	xmm1, [dZEROPNINE]
	mulsd	xmm0, xmm1
	cvtsd2ss	xmm2, xmm0
	movss	fxstep, xmm2
    
					;fystep = 0.9999 * fy;
	cvtss2sd	xmm0, fy
	movsd	xmm1, [dZEROPNINE]
	mulsd	xmm0, xmm1
	cvtsd2ss	xmm2, xmm0
	movss	fystep, xmm2
	
					;inimg += 54;
	add	inimg, 54
	
					;outimg += 54;
	add	outimg, 54
	
					;pdy = outimg;
	mov	rax, outimg
	mov	pdy, rax
	
					;const	
	movss	xmm4, [fHALF]

					;for (y=0; y <= destheight; ++y) {
	mov	y, 0
	jmp	forycondition
fory:
	
					;sy1 = fy * y;
        mov	eax, y
        pxor	xmm0, xmm0
        cvtsi2ss	xmm0, rax
        mulss	xmm0, fy
        movss	sy1, xmm0
        
					;jstart = (uint32_t)sy1;
        cvttss2si	rax, xmm0
        mov	jstart, eax
        
					;sy2 = sy1 + fystep;
        movss	xmm1, xmm0
        addss	xmm1, fystep
        movss	sy2, xmm1
        
        
					;jend = (uint32_t)sy2;
        cvttss2si	rax, xmm1
        mov	jend, eax
        
					;devY2 = jend+1-sy2;
        mov	eax, jend
        inc	eax
        pxor	xmm2, xmm2
        cvtsi2ss	xmm2, rax
        subss	xmm2, xmm1
        movss	devY2, xmm2
        
					;devY1 = 1-sy1+jstart;
        mov	eax, jstart
        inc	eax
        pxor	xmm2, xmm2
        cvtsi2ss	xmm2, rax
        subss	xmm2, xmm0
        movss	devY1, xmm2

					;pdx = pdy;
        mov	rax, pdy
        mov	pdx, rax

					;for (x=0; x <= destwidth; ++x) {
        mov	x, 0
        jmp	forxcondition
forx:
        
					;sx1 = fx * x;
        mov	eax, x
        pxor	xmm0, xmm0
        cvtsi2ss	xmm0, rax
        mulss	xmm0, fx
        movss	sx1, xmm0
        
					;istart = (uint32_t)sx1;
        cvttss2si	rax, xmm0
        mov	istart, eax
        
					;sx2 = sx1 + fxstep;
        movss	xmm1, xmm0
        addss	xmm1, fxstep
        movss	sx2, xmm1
        
					;iend = (uint32_t)sx2;
        cvttss2si	rax, xmm1
        mov	iend, eax
        
					;devX1 = 1-sx1+istart;
        mov	eax, istart
        inc	eax
        pxor	xmm2, xmm2
        cvtsi2ss	xmm2, rax
        subss	xmm2, xmm0
        movss	devX1, xmm2
        
					;devX2 = iend+1-sx2;
        mov	eax, iend
        inc	eax
        pxor	xmm2, xmm2
        cvtsi2ss	xmm2, rax
        subss	xmm2, xmm1
        movss	devX2, xmm2
        
					;destR = destG = destB = 0.0;
        movss	xmm0, [FZERO]
        movss	destR, xmm0
        movss	destG, xmm0
        movss	destB, xmm0
        
					;psj = inimg + (jstart*psStep);
        mov	eax, jstart
        imul	rax, psStep
        mov	rdx, rax
        mov	rax, inimg
        add	rax, rdx
        mov	psj, rax
        
					;dy = devY1;
	movss	xmm0, devY1
	movss	dy, xmm0
	
					;for (j=jstart; j <= jend; ++j) {
	mov	eax, jstart
	mov	j, eax
	jmp	forjcondition
forj:
	
					;if (j == jend)
        mov	eax, j
        cmp	eax, jend
        jne	if1
					;    dy = dy - devY2;
        movss	xmm0, dy
        subss	xmm0, devY2
        movss	dy, xmm0
                
					;dyf = dy * fiy;
if1:
	movss	xmm0, dy
	mulss	xmm0, fiy
	movss	dyf, xmm0
	
					;psi = psj + (istart * 3);
        mov	eax, istart
        mov	edx, eax
        add	rax, rdx
        add	rax, rdx
        add	rax, psj
        mov	psi, rax
	
					;dx = devX1;
	movss	xmm0, devX1
	movss	dxx, xmm0

					;for (i=istart; i <=iend; ++i)
        mov	eax, istart
        mov	i, eax
        jmp	foricondition
fori:
        
					;if (i == iend)
        mov	eax, iend
        cmp	eax, i
        jne	if2
        
					;    dxx = dxx - devX2;
	movss	xmm0, dxx
	subss	xmm0, devX2
	movss	dxx, xmm0
	
					;AP = dxx * dyf * fix;
if2:
	movss	xmm0, dxx
	mulss	xmm0, dyf
	mulss	xmm0, fix
	movss	AP, xmm0
	
					;destB += *(psi) * AP;
	mov	rax, psi
	movzx	eax, BYTE [rax]
	movzx	eax, al
	pxor	xmm0, xmm0
	cvtsi2ss	xmm0, rax
	mulss	xmm0, AP
	movss	xmm1, destB
	addss	xmm0, xmm1
	movss	destB, xmm0
	
					;destG += *(psi+1) * AP;
	mov	rax, psi
	movzx	eax, BYTE [rax+1]
	movzx	eax, al
	pxor	xmm0, xmm0
	cvtsi2ss	xmm0, rax
	mulss	xmm0, AP
	movss	xmm1, destG
	addss	xmm0, xmm1
	movss	destG, xmm0
	
					;destR += *(psi+2) * AP;
	mov	rax, psi
	movzx	eax, BYTE [rax+2]
	movzx	eax, al
	pxor	xmm0, xmm0
	cvtsi2ss	xmm0, rax
	mulss	xmm0, AP
	movss	xmm1, destR
	addss	xmm0, xmm1
	movss	destR, xmm0

					;psi += 3;
	add	psi, 3
	
					;dx = 1.0;
	movss	xmm0, [FLOAT_ONE]
	movss	dxx, xmm0
	
					;} // for i
	inc	i
foricondition:
	mov	eax, i
	cmp	eax, iend
	jbe	fori
                
					;psj += psStep;
        mov	rax, psStep
        add	psj, rax
        
					;dy = 1;
   	movss	xmm0, [FLOAT_ONE]
   	movss	dy, xmm0
   	
					;} // for j
        inc	j
forjcondition:
        mov	eax, j
        cmp	eax, jend
        jbe	forj

store:
					;*(pdx) = (uint8_t)(destB+0.5);
        movss	xmm0, xmm4		; move a half to xmm0
        addss	xmm0, destB
        cvttss2si	edx, xmm0
        mov	rax, pdx
        mov	BYTE [rax], dl
        
					;*(pdx+1) = (uint8_t)(destG+0.5);
        movss	xmm0, xmm4		; move a half to xmm0
        addss	xmm0, destG
        cvttss2si	edx, xmm0
        mov	rax, pdx
        mov	BYTE [rax+1], dl
            
					;*(pdx+2) = (uint8_t)(destR+0.5);
        movss	xmm0, xmm4		; move a half to xmm0
        addss	xmm0, destR
        cvttss2si	edx, xmm0
        mov	rax, pdx
        mov	BYTE [rax+2], dl

					;pdx += 3;
        add	pdx, 3
        
					;} // for x
        inc	x
forxcondition:
        mov	eax, x
        cmp	eax, destwidth
        jbe	forx
        
					;pdy += pdStep;
        mov	rax, pdStep
        add	pdy, rax
        
					;} // for y
   	inc	y
forycondition:
   	mov	eax, y
  	cmp	eax, destheight
  	jbe	fory
  		
					;}
	leave
	pop	r15
	pop	r14
	pop	r13
	pop	r12
	pop	r11
	pop	r10
	pop	rbx
	ret
