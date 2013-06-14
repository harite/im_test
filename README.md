an im for testing 100,000 concurrent tcp connections

usage:
1. route_server: ./route_server
   route_server will listen on port 8000 

2. im_server:
	./im_server im_server_ip route_server_ip
	if route_server is not on, the the im_server can work as a single server

3. im_client:
	./imclient im_server_ip src_start_id src_end_id dst_start_id dst_end_id pkt_per_second pkt_size


