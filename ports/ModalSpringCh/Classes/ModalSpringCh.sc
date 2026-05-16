ModalSpringCh : UGen {
	*ar { arg in = 0.0, pitch = 0.0, decay = 0.5, mix = 1.0, modModes = 0.0, modFreq = 1.0, modDepth = 0.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, pitch, decay, mix, modModes, modFreq, modDepth).madd(mul, add)
	}
}
