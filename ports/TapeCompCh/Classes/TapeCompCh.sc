TapeCompCh : UGen {
	*ar { arg in = 0.0, amount = 0.0, attack = 0.5, release = 0.5, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, amount, attack, release).madd(mul, add)
	}
}
