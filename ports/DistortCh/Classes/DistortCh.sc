DistortCh : Filter {
	*ar { arg in = 0.0, bass = 0.0, treble = 0.0, drive = 0.5, bias = 0.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, bass, treble, drive, bias).madd(mul, add)
	}
}
