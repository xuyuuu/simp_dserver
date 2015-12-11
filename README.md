#simp_dserver</br>   

===================================================================
Project Introduction   
===================================================================
This is a simple recursive DNS SERVER for handing A type request.   
1. Using epoll for handing dns request events through libevnet lib.    
2. Using multiple thread safety ring for dealing with data without a lock.    
3. Using a hashMap which was built with rbtree and list_head.    
4. Using a hash list for storing forwarding dns request.</br>   

==================================================================
Build and use   
==================================================================
1.Run 'make' commond to produce simp_dserver.   
2.Running 'simp_dserver -h' for help and run 'simp_dserver' to start dns service.   
Thanks for looking up !    
