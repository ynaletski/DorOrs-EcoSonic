
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
	
	set timeoutms 10
	set timeoutctr 0
	set reply ""
	while {$timeoutms >= $timeoutctr }  {
		binary scan [read $com] H* asciihex
		if {$asciihex != ""} {
			append reply $asciihex
			#puts [binary encode hex $reply]
			#puts $reply
		}
	after 1
	incr timeoutctr 1
	}
	if {$timeoutms < $timeoutctr && $reply == ""} {set reply "timeout"} 
	set state(Otvet_modbus) $reply
	
	if {$reply != "timeout"} {
		append count_byte [string range $reply [expr 4] [expr 5]]
		if {$count_byte == "04"} { set state(Otvet_modbus_) [necessary_float $reply 6 13] } elseif {$count_byte == "08"} { set state(Otvet_modbus_) [necessary_double $reply 6 21] }
	}
	#set state(Otvet_modbus_) [necessary_float $reply] 
	#set state(Otvet_modbus_) [necessary_double $reply]
}

proc close_com_modbus {} {
	global com
	close $com
}

proc open_com_modbus {} {
	global com
	set com [open {\\.\COM2} "RDWR"]
	#fconfigure $com -translation binary -mode 9600,n,8,1 -translation auto -buffering line -blocking 0
	fconfigure $com -translation binary -mode 38400,e,8,1 -translation binary -buffering line -blocking 0
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

proc necessary_float {my_var from to} {
	append otvet_obrez [string range $my_var [expr $from] [expr $to]]
	set count [string length $otvet_obrez]
	for {set x $count} {$x>0} {incr x -2} {
	append necessary_answer_for_tcl [string range $otvet_obrez [expr $x-2] [expr $x-1]]
	}
	set bin [binary decode hex  $necessary_answer_for_tcl]
	binary scan $bin f float_answer
	return $float_answer
}

proc necessary_double {my_var from to} {
	append otvet_obrez [string range $my_var [expr $from] [expr $to]]
	set count [string length $otvet_obrez]
	for {set x $count} {$x>0} {incr x -2} {
	append necessary_answer_for_tcl [string range $otvet_obrez [expr $x-2] [expr $x-1]]
	}
	set bin [binary decode hex  $necessary_answer_for_tcl]
	binary scan $bin d double_answer
	return $double_answer
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
	#console show
	#puts "console show"
	set state(listner) "1"
	wait_request
}

proc listen_Off {} {
	global state
	#puts "console hide"
	#console hide
	set state(listner) "0"
	
}

proc wait_request {} {
	global com
	global state

	switch $state(listner) {
		"0" { return }
		"1" { 
				global com
				global state
				append message [binary format H* "0a0305ea0006"]
				set checksum [::crc::crc16 -seed 0xFFFF $message]
				set checksum [binary format s $checksum] 
				append message $checksum
				puts -nonewline $com $message
				flush $com	
				set timeoutms 10
				set timeoutctr 0
				set reply ""
				while {$timeoutms >= $timeoutctr }  {
					binary scan [read $com] H* asciihex
					if {$asciihex != ""} {
						append reply $asciihex
					}
					after 1
					incr timeoutctr 1
				}
				if {$timeoutms < $timeoutctr && $reply == ""} {set reply "timeout"} 
				set state(Otvet_modbus) $reply
				if {$reply != "timeout"} {
					set state(Otvet_modbus_) [format %.4f [necessary_float $reply 6 13]] 
					set state(f_t_c) [format %.2f [necessary_float $reply 14 21]] 
					set state(f_pr_bar) [format %.4f [necessary_float $reply 22 29]] 
				} else { set state(Otvet_modbus_) $reply 
					set state(f_t_c) $reply 
					set state(f_pr_bar) $reply 
				}
				##################################################################
				append message_2 [binary format H* "0a03065c000c"]
				set checksum [::crc::crc16 -seed 0xFFFF $message_2]
				set checksum [binary format s $checksum] 
				append message_2 $checksum
				puts -nonewline $com $message_2
				flush $com	
				set timeoutms_2 10
				set timeoutctr_2 0
				set reply_2 ""
				while {$timeoutms_2 >= $timeoutctr_2 }  {
					binary scan [read $com] H* asciihex_2
					if {$asciihex_2 != ""} {
						append reply_2 $asciihex_2
					}
					after 1
					incr timeoutctr_2 1
				}
				if {$timeoutms_2 < $timeoutctr_2 && $reply_2 == ""} {set reply_2 "timeout"} 
				set state(Zapros_modbus) $reply_2
				if {$reply_2 != "timeout"} {
					set state(Zapros_modbus_) [format %.4f [necessary_double  $reply_2 6 21]] 
					set state(d_t_c) [format %.2f [necessary_double $reply_2 22 37]]
					set state(d_pr_bar) [format %.4f [necessary_double $reply_2 38 53]]
				} else { set state(Zapros_modbus_) $reply_2 
					set state(d_t_c) $reply_2 
					set state(d_pr_bar) $reply_2 
				}
		}
		default {break}
	}
	after 300 wait_request
}

#тестирование таймера
proc do {} {
    puts "123123321"
    after 1000 do
}
