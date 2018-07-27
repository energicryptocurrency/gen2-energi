
Vagrant.configure("2") do |config|
    config.vm.define "builder" do |node|
        node.vm.provider "virtualbox" do |v|
            # TODO: better detect from host system
            # - half RAM
            # - all CPUs
            # "safe" defaults for any modern developer's system
            v.memory = ENV.fetch('VM_MEMORY', 4096)
            v.cpus = ENV.fetch('VM_CPUS', 4)
        end
        node.vm.box = "bento/ubuntu-16.04"

        #node.vm.network "forwarded_port", guest: 19999, host: 19999, host_ip: "127.0.0.1"
        #node.vm.network "forwarded_port", guest: 9999, host: 9999, host_ip: "127.0.0.1"

        node.vm.provision 'libdb4', type: "shell", inline:\
            "add-apt-repository ppa:bitcoin/bitcoin;"\
            "apt-get update;"\
            "apt-get install -y libdb4.8-dev libdb4.8++-dev"

        node.vm.provision 'cid', type: "shell", inline:\
            "apt-get install -y python3-pip;"\
            "/usr/bin/pip3 install -U futoin-cid"
        
        node.vm.synced_folder(".", "/vagrant",
            type: 'virtualbox',
            owner: 'vagrant', group: 'vagrant',
            create: true
        )
    end
end
