# RERANZ
RERANZ: A Light-Weight Virtual Machine to Mitigate Memory Disclosure Attacks (See Paper VEE'2017)
<br /> http://dl.acm.org/citation.cfm?id=3050752&CFID=986069799&CFTOKEN=34911294 <br/>

# Abstract 
Recent code reuse attacks are able to circumvent various address space layout randomization (ASLR) techniques by exploiting 
memory disclosure vulnerabilities. To mitigate sophisticated code reuse attacks, we proposed a light-weight virtual machine, 
RERANZ, which deployed a novel continuous binary code re-randomization to mitigate memory disclosure oriented attacks. In 
order to meet security and performance goals, costly code randomization operations were outsourced to a separate process, 
called the “shuffling process”. The shuffling process continuously flushed the old code and replaced it with a fine-grained 
randomized code variant. RERANZ repeated the process each time an adversary might obtain the information and upload a payload. 
Our performance evaluation shows that RERANZ Virtual Machine incurs a very low performance overhead. The security evaluation 
shows that RERANZ successfully protect the Nginx web server against the Blind-ROP attack.

# Note
Note that we only upload partial source code of RERANZ, it is not the final implementation of RERANZ. We had label the TODO 
functions in the source code. If you want to use this source code, you need to implement it by yourself.
