CentaurCh : UGen {
	*ar { arg in = 0.0, gain = 0.5, treble = 0.5, level = 0.5, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, gain, treble, level).madd(mul, add)
	}
}
