The http server here was evolved for use of internal services from the boost asio_http server3 example.  Modifications were made to extend it for a more favorable use case as a REST services container in c++11.  All credit goes to boost_asio maintainer for the original code, modifications were only for the handling of POST data and parameters as well pluggable web service ports and multimap's instead of vectors for http parameters.

Examples of usage are in server.cpp.  Take a look at the echo handler and server_status handler.   Start up the service & hit /server-status or /echo to see results.

Input/hupdates/improvements welcome.  I have a limited internal use case I use this for, it would be interesting to see a more fully evolved system that doesn't piggyback the various apache systems.

-V
