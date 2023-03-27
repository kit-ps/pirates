# Pirates

Repository for proof-of-concept and measurement code for the PIRATES! protocol

## Evaluation Notes

We want to investigate the following hypothesis through empirical evaluation:

**Pirates enables group calls with a mouth-to-ear latency of less than 400 ms.**

### Mouth-to-Ear Latency

It is important that we evaluate not only *transmission* latency, but also *mouth-to-ear* (MTE) latency (i.e., the time between when a word is said by one participant and when it is heard by another).
In addition to transmission latency, this includes the following

1. Recording and playback
2. Encoding and decoding

We should be able to easily add encoding/decoding steps (of fixed voice data) to our existing prototype.
More difficult to evaluate is the latency added by the audio stack of the sending and receiving devices, i.e. the time to record and playback the audio, since this depends on the actual devices and requires physical measurements.
Research by Superpowered Inc. (see [superpowered.com](https://superpowered.com/superpowered-android-media-server)) suggests that a modern Android or iOS device adds 10 ms of latency for recording and playback.
Therefore, we add an offset of 10 ms to all measurements to account for recording and playback latency.

### System Parameters/Snippet Length

Prior to the main experiments, we need to determine the optimal system parameters for Pirates, especially the snippet/subground length:

- The longer the snippet, the more of the total latency is taken up by recording and playback.
- The shorter the snippet, the more the system's "base overhead" becomes critical:
        Below a certain threshold, the system will not have enough time to process and distribute the current snippets before the next ones have to be processed.

The optimal snippet length is likely to depend on both the total number of users and the group size (as well as other factors such as server performance and network latency). 

### Related Work/Baseline

We compare Pirates to related work (e.g., Addra) in Section 8.3 (worker scalability) and in Section 8.4. (dialing).
In this evaluation, we'll examine Pirates by itself.
This makes sense, because for a fixed number of workers the only latency relevant difference between Pirates and Group-Addra is that the voice snippet forwarding is parallelized in Pirates by adding relays.

Therefore, we only test Pirates to see if the architecture can support low-latency communication, and investigate the benefits of Pirates separately.

### Setup 

To test Pirates' MTE latency, we set up a client, a master, a relay, and a worker.
The client acts as both sender and receiver of a test message.
Data and requests for additional clients are precomputed and used to simulate additional load on the master/relay/worker in a repeatable and efficient manner.
In Section 8.3, we determined that a good ratio between relays and workers is 1:20 (one relay for every 20 workers).
So we use the same ratio here (which determines how much data the single relay has to forward).
To be consistent with Addra's evaluation, we assume 80 workers, 4 relays, and one master (only one of each is actually run).
A test run includes the following steps

1. Client encodes and encrypts the (dummy) voice snippet.
2. Client sends encrypted voice snippet (with mailbox ID and auth token) to relay
3. Relay has `#clients/#relays` voice snippets.
    Relay verifies auth token for each.
4. Relay forwards all voice data to worker (and additional dummy workers)
5. Worker assembles database
6. Worker has `#clients*(group size - 1)/#workers` requests answer.
    Computes all replies.
7. Worker sends reply to client (and other replies to dummy clients)
8. Client decrypts and decodes voice snippet

For realistic values, we must ensure that the real client/worker does not receive their data first, but is at a random position within the dummies.

### Experiments

We want to find out not only whether Pirates can achieve sub-400ms MTE latency *at any cost*, but also how efficiently it can do so.
The more "communications" the system can handle with a given number of servers (while maintaining sub-400ms MTE latency), the more efficient it is. 
The amount of "communications" depends on both the number of participants in the system and the size of the group.
So we test the impact of each:

#### Number of Participants

Set the group size to 8, increase the number of participants until the MTE latency exceeds 400 ms.

#### Group Size

Set the number of participants to TBD and increase the group size until the MTE latency exceeds 400 ms.
The number of participants for this experiment should be chosen so that the MTE latency is *well below* 400 ms for the minimum group size tested.
