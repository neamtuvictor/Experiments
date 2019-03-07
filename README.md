# Each test produced will be in it's own folder

 1) SharingSameCore test creates 1 producer thread, 2 consumers and 1 logger. The logger purpose is only to avoid seeing the overhead of cout. If the 2 consumers threads stay on same core, latency increases.
 2) SHM ProcessCommunication has 2 different apps, each on it's own process. Producer generates data, then one or multiple consumers will read it independently. If 2 or multiple consumers on the same core, we will see some latency spikes. 
On my machine i get ~120ns to pass the data between the processes.
 
