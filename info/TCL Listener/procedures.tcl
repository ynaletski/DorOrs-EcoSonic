
########################################################################
########################################################################
####################        часть модбаса        #######################
########################################################################
########################################################################
########################################################################
proc send_modbus {} {
	global com
	global state
#	puts $com $state(Zapros)
#	fileevent $com readable [list rd $com]
	append message [binary format H* $state(Zapros_modbus)]
	set checksum [::crc::crc16 -seed 0xFFFF $message]
	set checksum [binary format s $checksum] 
	append message $checksum
	puts -nonewline $com $message
	flush $com
	
	set timeoutms 500
	set timeoutctr 0
	set reply ""
	while {$timeoutms >= $timeoutctr }  {
		binary scan [read $com] H* asciihex
		if {$asciihex != ""} {
			append reply $asciihex
			#puts [binary encode hex $reply]
			#puts $reply
		}
	after 5
	incr timeoutctr 5
	}
	if {$timeoutms < $timeoutctr && $reply == ""} {set reply "timeout"} 
	set state(Otvet_modbus) $reply	
}
proc close_com_modbus {} {
	global com
	close $com
}
proc open_com_modbus {} {
	global com
	set com [open {\\.\COM2} "RDWR"]
	fconfigure $com -translation binary -mode 19200,n,8,1 -translation auto -buffering line -blocking 0
}
########################################################################
########################################################################
##########     часть кодирование/декодирование float/hex     ###########
########################################################################
########################################################################
########################################################################
proc float_to_hex {var_float} {
	set bin [binary format f $var_float]
	set var_hex [binary encode hex $bin]
	return var_hex
}
proc hex_to_float {var_hex} {
	set bin [binary decode hex  $var_hex]
	[binary scan $bin f var_float]
	return var_float
}
########################################################################
########################################################################
###############        часть модбаса Слушатуль       ###################
########################################################################
########################################################################
########################################################################
set state(listner) "0"
proc listen_On {} {
	global state
	console show
	puts "console show"
	set state(listner) "1"
	wait_request
}
proc listen_Off {} {
	global state
	puts "console hide"
	#console hide
	set state(listner) "0"
	
}
proc wait_request {} {
	global com
	global state
	switch $state(listner) {
		"0" {}
		"1" { 
			puts "wait"
			binary scan [read $com] H* asciihex
			set reply ""
			if {$asciihex != ""} {
				append reply $asciihex
				set state(Zapros_modbus_) $reply
				puts "$reply"
				set $reply " "
			}	
		}
		default {break}
	}
	
	set otvet "00000000"
	set checksum [::crc::crc16 -seed 0xFFFF $otvet]
	set checksum [binary format s $checksum] 
	append otvet $checksum
	puts -nonewline $com $otvet
	flush $com
	puts "$otvet"
	
	after 170 wait_request
}
#тестирование таймера
proc do {} {
    puts "123123321"
    after 1000 do
}
