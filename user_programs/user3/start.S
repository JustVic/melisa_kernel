.text
.global start
.extern main

start:
        call main
		call exitu
		jmp .
