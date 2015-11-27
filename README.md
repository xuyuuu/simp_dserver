#simp_dserver</br>   
This is a simple recursive DNS SERVER for handing A type request.   
1. Using epoll for handing dns request events through libevnet lib.    
2. Using multiple thread safety ring for dealing with data without a lock.    
3. Using a hashMap which was built with rbtree and list_head.    
4. Using a hash list for storing forwarding dns request.</br>   
Running 'netdispatch -h' for help......    
Thanks for looking up !    
