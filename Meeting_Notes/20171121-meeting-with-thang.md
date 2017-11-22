Meeting with Client, 2017-11-21

Attendees:
* Thang Hoang
* Andrew Ekstedt
* Scott Russell
* Scott Merrill (remote)

We talked about our tech reports (wrapping up) and design document (just starting).

Tech review status update:
* AE: turned in my tech report. 
* SR: almost finished, need to convert to latex and turn in
* SM: almost finished, need to finish up MAC section and turn in

Tech review conclusions:
* SR
    * email protocol: POP3
    * cloud computing: EC2
    * also designed benchmarking methodology
* AE
    * encryption algorithm: AES-CTR
    * cloud storage: S3
    * also reviewed SSE algorithms
* SM
    * language: C++
    * crypto library: tomcrypt
    * MAC scheme: HMAC probably
    
SM: could use some help finding other MAC algorithms to compare HMAC.
    tomcrypt supports a whole bunch of them. PMAC, CMAC, BLAKE2B...
AE: i've heard of BLAKE2B. it's faster than HMAC but not as well studied.
    it might be a good candidate
    
Client verification of tech review not due til next week.
Also, client only needs to verify receipt of the tech review, not approval.
We'll send out our final drafts in a couple days

Design discussion:
* The central component of the system will be a C++ library that implement's Cash's DSSE scheme
* There will be a basic client-server program that uses the SSE module
* There will be a daemon which downloads email over POP3 and adds it to the encrypted search index using the SSE module
* Research question: index size
    * Implement Cash's 2lev ptr optimization.
    * Which parameters (eg block size) yield the optimum index size?
* Research question: parallelization
    * Which parts of Cash-DSSE are parallelizable?
    * If a part is parallelizable, attempt to implement it
    * If a part is not parallelizable, document why not
* Reasearch question: dumb server
    * Can the optimized Cash algorithm be used if the server just provides storage and not computation?
    * If yes, attempt to implement
    * If no, identify why not. Can the scheme be tweaked?
    
Design doc due Dec 1, with client approval.
We'll have a draft ready sometime next week.It would be good to meet with Attila to go over it.

Action items:
* SR&SM will turn in tech review today
* AE will send meeting notes tonight
* SR will email Attila to set up meeting next week
* SR will email Winters to ask for a design doc example
* AE will work on design doc first draft on Thursday or Friday and send to Winters for feedback
* Someone will send Attila our tech reviews
