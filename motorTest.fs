

: clear pa0 ioc! pa1 ioc! pa2 ioc! ;
: setup-Motor
  omode-pp  pa0 io-mode!
  omode-pp  pa1 io-mode!
  omode-pp  pa2 io-mode!
  clear ;

: delay 10 ms ;
: toggle dup ios! delay ioc! delay ;
: setMotor pa2 io! pa1 io! pa0 io! ;
: motor
  begin

    pa0 toggle
    pa1 toggle
    pa2 toggle

    delay
    key? until ;

: motor2
  1
  begin
    1 0 0 setMotor dup ms
    1 1 0 setMotor dup ms
    0 1 0 setMotor dup ms
    0 1 1 setMotor dup ms
    0 0 1 setMotor dup ms
    1 0 1 setMotor dup ms
    1 + 25 mod
    key? until
  clear
;
setup-Motor

