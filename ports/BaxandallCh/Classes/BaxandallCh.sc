BaxandallCh : UGen {
	*ar { arg in = 0.0, bass = 0.5, treble = 0.5, mode = 0.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, bass, treble, mode).madd(mul, add)
	}
}
