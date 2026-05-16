AutoWahCh : UGen {
	*ar { arg in = 0.0, freq = 500.0, q = 5.0, gainDB = 0.0, attackMs = 1.0, releaseMs = 50.0, freqMod = 0.5, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, freq, q, gainDB, attackMs, releaseMs, freqMod).madd(mul, add)
	}
}
