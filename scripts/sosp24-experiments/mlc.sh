sudo bash run-dctcp-tput-experiment.sh -E "mlc0-$1" -M 4000 --num_servers 5 --num_clients 5 -c "4,8,12,16,20" --ring_buffer 256 --buf 1 --mlc_cores 'none' --bandwidth "100g" --server_intf ens2f1np1
sudo bash run-dctcp-tput-experiment.sh -E "mlc1-$1" -M 4000 --num_servers 5 --num_clients 5 -c "4,8,12,16,20" --ring_buffer 256 --buf 1 --mlc_cores '1' --bandwidth "100g" --server_intf ens2f1np1
sudo bash run-dctcp-tput-experiment.sh -E "mlc2-$1" -M 4000 --num_servers 5 --num_clients 5 -c "4,8,12,16,20" --ring_buffer 256 --buf 1 --mlc_cores '1,2' --bandwidth "100g" --server_intf ens2f1np1
sudo bash run-dctcp-tput-experiment.sh -E "mlc3-$1" -M 4000 --num_servers 5 --num_clients 5 -c "4,8,12,16,20" --ring_buffer 256 --buf 1 --mlc_cores '1,2,3' --bandwidth "100g" --server_intf ens2f1np1
