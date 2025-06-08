# PAS_emulator
Some ebikes start up engine when pedalling backwards, which is dangerous. 
this system simply allows replacing such a sensor with a throttle sensor. 
Also useful when PAS sensor is damaged or if one needs ebike startup without pedals (f.e. for disabled person - locked knee etc).

AI generated (various AIs)

UBI AI license. 

sleep version without pcint requires a jumper from throttle pin to pin 2 (interrupt)
PCINT version requires no extra wiring, it wakes up on PCINT set to A0 input , but it requires library :
https://github.com/paulo-raca/YetAnotherArduinoPcIntLibrary
