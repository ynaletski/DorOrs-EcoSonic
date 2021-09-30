#set var_hex 3b4390d4
#puts $var_hex
#set bin [binary decode hex  $var_hex]
#puts $bin
#binary scan $bin f var_float
#puts $var_float

#3c8d0774

############            перестановка символов            ###############
#set var_no_convert 4194b333
#puts "Было  $var_no_convert"
#set bin1 [binary decode hex  $var_no_convert]
#binary scan $bin1 f var_float1
#puts $var_float1
#set count [string length $var_no_convert]
#for {set x $count} {$x>0} {incr x -2} {
#puts [string range $var_no_convert [expr $x-2] [expr $x-1]]
#append var_convert [string range $var_no_convert [expr $x-2] [expr $x-1]]
#}
#puts "Стало $var_convert"
#set bin2 [binary decode hex  $var_convert]
#binary scan $bin2 f var_float2
#puts $var_float2
########################################################################
#set var 123456789987654321
#puts "Было  $var"
#append var_obrez [string range $var [expr 6] [expr 13]]
#puts "Стало $var_obrez"
########################################################################

puts "!!!!!!!!!!!!!!!!!!!!!\n\n"

set otvet 0a0304b3334194a787
puts "response $otvet"
append count_byte [string range $otvet [expr 4] [expr 5]]
puts $count_byte
if {$count_byte == "04"} { puts 1 } elseif {$count_byte == "08"} { puts 2 }

#trim b3334194 part
append otvet_obrez [string range $otvet [expr 6] [expr 13]]
puts "trim to $otvet_obrez"

#changes siquence on 4194b333  (for https://gregstoll.com/~gregstoll/floattohex/)
append one [string range $otvet_obrez [expr 4] [expr 7]]
append two [string range $otvet_obrez [expr 0] [expr 3]]
append necessary_answer_for_site $one $two
puts "$necessary_answer_for_site changes siquence for https://gregstoll.com/~gregstoll/floattohex/"

#changes siquence on 33b39441 (for tcl)
set count [string length $necessary_answer_for_site]
for {set x $count} {$x>0} {incr x -2} {
append necessary_answer_for_tcl [string range $necessary_answer_for_site [expr $x-2] [expr $x-1]]
}
puts "$necessary_answer_for_tcl changes siquence for tcl"

#hex to float
set bin [binary decode hex  $necessary_answer_for_tcl]
binary scan $bin f float_answer
puts "Float $float_answer"

puts "\n\n!!!!!!!!!!!!!!!!!!!!!\n\n"

##############################################################################################3

set otvet 0a0308147b3fefb645a1ca6589
puts "response $otvet"

#trim 147b3fefb645a1ca part
append otvet_obrez [string range $otvet [expr 6] [expr 21]]
puts "trim to $otvet_obrez"

#changes siquence on 3fef147ba1cab645  (for https://gregstoll.com/~gregstoll/floattohex/)
append one [string range $otvet_obrez [expr 4] [expr 7]]
append two [string range $otvet_obrez [expr 0] [expr 3]]
append three [string range $otvet_obrez [expr 12] [expr 15]]
append four [string range $otvet_obrez [expr 8] [expr 11]]
append necessary_answer_for_site $one $two $three $four
puts "$necessary_answer_for_site changes siquence for https://gregstoll.com/~gregstoll/floattohex/"

#changes siquence on 45b6caa17b14ef3f (for tcl)
set count [string length $necessary_answer_for_site]
for {set x $count} {$x>0} {incr x -2} {
append necessary_answer_for_tcl [string range $necessary_answer_for_site [expr $x-2] [expr $x-1]]
}
puts "$necessary_answer_for_tcl changes siquence for tcl"

#hex to double
set bin [binary decode hex  $necessary_answer_for_tcl]
binary scan $bin d double_answer
puts "Double $double_answer"

puts "\n\n!!!!!!!!!!!!!!!!!!!!!\n\n"

###########################################################################





set x 147b3fefb645a1ca
set bin1 [binary decode hex $x]
binary scan $bin1 d dou
puts "double $dou"

set x 3fef147ba1cab645
set bin1 [binary decode hex $x]
binary scan $bin1 d dou
puts "double $dou"

set x 45b6caa17b14ef3f
set bin1 [binary decode hex $x]
binary scan $bin1 d dou
puts "double $dou"
