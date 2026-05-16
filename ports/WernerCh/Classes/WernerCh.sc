WernerCh : UGen {
	*ar { arg in = 0.0, freq = 632.4555, feedback = 0.5, damping = 0.5, drive = 0.1, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, freq, feedback, damping, drive).madd(mul, add)
	}
}
