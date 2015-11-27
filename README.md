#netdispatch</br>   
</br>   
</br>   
This is a simple recursive DNS SERVER for handing A type request.</br>   
</br>   
1. Using epoll for handing dns request events through libevnet lib.</br>   
</br>   
2. Using multiple thread safety ring for dealing with data without a lock.</br>   
</br>   
3. Using a hashMap which was built with rbtree and list_head.</br>   
</br>   
4. Using a hash list for storing forwarding dns request.</br>   
</br>   
Running 'netdispatch -h' for help......</br>   
Thanks for looking up !</br>    
