ChorusCh : MultiOutUGen {
	*ar { arg in = 0.0, rate = 0.5, depth = 0.5, feedback = 0.0, mix = 0.5;
		^this.multiNew('audio', 2, in, rate, depth, feedback, mix)
	}

	init { arg numChannels ... theInputs;
		inputs = theInputs;
		^this.initOutputs(numChannels, rate);
	}

	argNamesInputsOffset { ^2 }
}
