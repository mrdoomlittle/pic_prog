	processor 16f877a
	include p16f877a.inc
	__config _HS_OSC & _WDT_OFF & _LVP_ON & _PWRTE_ON
	org 0
start
	bsf STATUS,RP0
	bcf STATUS,RP1

	movlw b'00000000'
	movwf TRISD

	bcf STATUS,RP0
	bcf STATUS,RP1
	movlw b'00001111'
	movwf PORTD
	goto start

	end
