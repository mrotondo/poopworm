var synthDefPath = "~/poopworm/synthdefs".standardizePath;
SynthDef.synthDefDir = synthDefPath;
~synthPath = "~/poopworm/Synths/*".standardizePath;
~effectPath = "~/poopworm/Effects/*".standardizePath;

SynthDef(\OutConnector, {|inBus|
	Out.ar(0, In.ar(inBus));
}).load(s);

~soundWormSynthCreator = {|name, synthUGen|
    // Creates appropriately enveloped synths for percussion sounds
    SynthDef(name, {|outBus=0, pitch=440, duration=0.01|
	    var gate = Trig.kr(1, duration);
        var volumeEnvelope = Linen.kr(
	        attackTime:0.01,
            gate:gate,
            releaseTime:0.2,
            doneAction:2
        );
        var synth = Pan2.ar(
            synthUGen.(pitch),
            pos:Rand(-1, 1)
        );
        Out.ar(outBus, volumeEnvelope * synth);
    }).load(s);
};

~soundWormEffectCreator = {|name, synthUGen|
    // Creates appropriately enveloped synths for percussion sounds
    SynthDef(name, {|inBus=0, pitch=0|
	    var input = In.ar(inBus, 1);
        var synth = Pan2.ar(
            synthUGen.(input),
            pos:0
        );
        ReplaceOut.ar(inBus, synth);
    }).load(s);
};

fork {
    s.bootSync;
    
    //~loadTestSynth.();
    //~loadStressSynth.(200);
    
    ~synthPath.pathMatch.do{|filePath|
	    var synthName = filePath.basename.splitext[0];
	    "loading ".post; filePath.postln;
        ~soundWormSynthCreator.(synthName, filePath.load);
    };
    
    ~effectPath.pathMatch.do{|filePath|
	    var synthName = filePath.basename.splitext[0];
	    "loading ".post; filePath.postln;
        ~soundWormEffectCreator.(synthName, filePath.load);
    };
    
    //0.exit; // Exit SuperCollider at end of compile script
}
