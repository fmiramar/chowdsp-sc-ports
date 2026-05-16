PolygonalOscCh : UGen {
	*ar { arg freq = 1000.0, order = 3.5, teeth = 0.0, gainDB = -24.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', freq, order, teeth, gainDB).madd(mul, add)
	}
}
