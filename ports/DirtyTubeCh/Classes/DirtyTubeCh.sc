DirtyTubeCh : UGen {
	*ar { arg in = 0.0, drive = 0.5, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, drive).madd(mul, add)
	}
}
