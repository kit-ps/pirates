# Pirates

Repository for proof-of-concept and measurement code for the PIRATES! protocol

## Evaluation Notes

The overarching question we need to answer through empirical evaluation is:

**Does Pirates offer better *scalability* than related work while achieving the ITU's recommended maximum mouth-to-ear latency of 400 ms?**

### Mouth-to-Ear Latency

It is important that we evaluate not only *transmission* lantency, but the *mouth-to-ear* (MTE) latency (i.e., the time between when a word is said by one participant and when it is heard by another).
In addition to the transmission latency, this includes the following:

1. Recording and playback
2. Encoding and decoding

We should be easily able to add encoding/decoding steps (of fixed voice data) to our existing prototype.
It is harder to evaluate is the latency added by the audio stack of the sending and receiving devices, i.e., the time to record and playback the audio, since that depends on the actual devices and requires physical measurements.
Research by Superpowered Inc. (see [superpowered.com](https://superpowered.com/superpowered-android-media-server)) suggest that a modern Android or iOS device adds 10 ms of latency for recording and playback.
We therefore add an offset of 10 ms to all measurements to account for recording and playback latency.

### System Parameters/Snippet Length

Prior to the main experiments, we need to determine optimal system parameters for Pirates, especiall the snippet/subround length:

- The longer the snippet, the more of the total latency is taken up by recording and playback
- The shorter the snippet, the more the 'base overhead' of the system will be critical:
        Under a certain threshold length, the system will not have enough time to process and distribute the current snippets before the next ones have to be processed.

Optimal snippet length is probably both dependent on the total number of users as well as the group size (and other factors such as server performance and network latency). 

### Related Work/Baseline

We will compare Pirates against a naive group extension of Addra, where group communication is enabled via a series of unicasts.
We call this baseline *Group-Addra*.

### Setup 

To test Pirates' and Group-Addra's MTE latency, we set up the one client, master, relay (for Pirates), and worker each.
The client acts as both sender and reciever of a test message.
Data and requests for additional clients will be pre-computed and used to simulate additional load on master/relay/worker in a repeatable and efficient fashion.

### Experiments

- Vary number of users
- Vary group size
- How many workers?
- TBD
