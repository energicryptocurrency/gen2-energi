
Vagrant.configure("2") do |config|
    config.vm.define "builder" do |node|
        node.vm.provider "virtualbox" do |v|
            # TODO: better detect from host system
            # - half RAM
            # - all CPUs
            # "safe" defaults for any modern developer's system
            v.memory = ENV.fetch('VM_MEMORY', 4096)
            v.cpus = ENV.fetch('VM_CPUS', 4)
            v.gui = true
        end
        node.vm.box = "bento/ubuntu-18.04"

        node.vm.network "forwarded_port", guest: 19796, host: 19796, host_ip: "127.0.0.1"
        #node.vm.network "forwarded_port", guest: 9999, host: 9999, host_ip: "127.0.0.1"

        node.vm.provision 'libdb4', type: "shell", inline:\
            "add-apt-repository ppa:bitcoin/bitcoin;"\
            "apt-get update;"\
            "apt-get install -y libdb4.8-dev libdb4.8++-dev"

        node.vm.provision 'cid', type: "shell", inline:\
            "apt-get install -y python3-pip;"\
            "/usr/bin/pip3 install -U futoin-cid"

        node.vm.provision 'git', type: "shell", inline:\
            "apt-get install -y --no-install-recommends git;"\
            "sudo -H -u vagrant git config --global user.name '#{`git config --global user.name`}';"\
            "sudo -H -u vagrant git config --global user.email '#{`git config --global user.email`}'"

        node.vm.provision 'xorg', type: "shell", inline:\
            "echo 'nodm    nodm/enabled    boolean true' | debconf-set-selections;"\
            "apt-get install -y --no-install-recommends xorg fluxbox nodm;"
        
        node.vm.provision 'bashrc', type: "shell", inline:\
            "ensure_bashrc() { grep -q \"$@\" /home/vagrant/.bashrc || (echo \"$@\" >> /home/vagrant/.bashrc )};"\
            "ensure_bashrc 'cd /vagrant';"\
            "ensure_bashrc  'export DISPLAY=\":0\"';"
        
        node.vm.synced_folder(".", "/vagrant",
            type: 'virtualbox',
            owner: 'vagrant', group: 'vagrant',
            create: true
        )
        
        VM_ENERGICORE_DATA = ENV.fetch('VM_ENERGICORE_DATA', '../energicore_vagrant_data')
        FileUtils.mkdir_p VM_ENERGICORE_DATA

        node.vm.synced_folder(VM_ENERGICORE_DATA, "/home/vagrant/.energicore",
            type: 'virtualbox',
            owner: 'vagrant', group: 'vagrant',
            create: true
        )
    end
end
