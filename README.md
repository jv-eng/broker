# BROKER
## Introduction
The following repository contains different versions of a communication broker that acts as an intermediary, offering a library to allow different processes to connect to that broker.

## Brokers
##### ZeroCopy
<p>Broker that acts as an intermediary for different processes. No message is stored.</p>
<p>Queue based broker. Client-fail tolerant.</p>
<p>Operations:</p>
<ul>
<li>Create: creates a queue.</li>
<li>Destroy: destroys a queue.</li>
<li>Put: send a message to the broker and store in an existent queue.</li>
<li>Get: gets a message from a queue. If there is no message, the process can wait for one to arrive.</li>
</ul>

#### EDSU
<p>Publisher-subscriber system. Uses a broker as an intermediary.</p>