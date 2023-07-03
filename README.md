# ZeroCopy

## Characteristics
<p>Broker that acts as an intermediary for different processes. No message is stored.</p>
<p>Queue based broker. Client-fail tolerant.</p>

## Operations
<ul>
<li>Create: creates a queue.</li>
<li>Destroy: destroys a queue.</li>
<li>Put: send a message to the broker and store in an existent queue.</li>
<li>Get: gets a message from a queue. If there is no message, the process can wait for one to arrive.</li>
</ul>