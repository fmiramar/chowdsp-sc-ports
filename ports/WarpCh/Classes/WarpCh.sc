WarpCh : UGen {
	*ar { arg in = 0.0, cutoff = 0.5, heat = 0.5, width = 0.5, drive = 0.0, mode = 0.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, cutoff, heat, width, drive, mode).madd(mul, add)
	}
}
