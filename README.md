# Pirates

Repository for proof-of-concept and measurement code for the PIRATES! protocol

## Evaluation Notes

We want to investigate the following hypothesis through empirical evaluation:

**Pirates enables group calls with a mouth-to-ear latency of less than 400 ms.**

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

We compare Pirates to related work (i.e., Addra) in Sec. 8.3 (worker scalability) and and Sec. 8.4. (dialing).
In this evaluation, we investigate Pirates on it's own.
This is sensible, as --- with a fixed number of workers -- the only latency-relevant difference between Pirates and Group-Addra is that the voice data forwarding is parallelized in Pirates through the addition of relays.

Thus, we only test Pirates to see if the architecture can support low lantency communication and investigate the advantages of Pirates separately.

### Setup 

To test Pirates' MTE latency, we set up the one client, master, relay (for Pirates), and worker each.
The client acts as both sender and reciever of a test message.
Data and requests for additional clients will be pre-computed and used to simulate additional load on master/relay/worker in a repeatable and efficient fashion.
In Sec. 8.3., we determine that a good ratio between relays and workers is 1:20 (one relay for every 20 workers).
We thus use the same ratio here (which determine how much data the single relay has to forward).
An experiment run includes the following steps:

1. Client encodes and encrypts the (dummy) voice snippet
2. Client sends encrypted voice snippet (with mailbox ID and auth token) to relay
3. Relay has #clients/#relays voice snippets.
    Relay checks auth token for each.
4. Relay forwards all voice data to worker (as well as additional dummy workers)
5. Worker assembles whole database
6. Worker has #clients*(group size - 1)/#workers requests answer.
    Computes all replies.
7. Worker sends reply to client (and other replies to dummy clients)
8. Client decrypts and decodes the voice snippet

For realistic values, we need to ensure that the real client/worker do not receiver their data first, but are rather at a random position within the dummies.

### Experiments

- Vary number of users
- Vary group size
- How many workers?
- TBD
