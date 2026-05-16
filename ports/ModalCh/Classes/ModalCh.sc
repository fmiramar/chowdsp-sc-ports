ModalCh : Filter {
	*ar { arg in = 0.0, freq = 440.0, decay = 1.4, amp = 0.25, phase = 0.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, freq, decay, amp, phase).madd(mul, add)
	}
}
