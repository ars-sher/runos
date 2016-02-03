#  task number 4:{2;2;4}

from mininet.topo import Topo

class NatTopology4 (Topo):
    "topology number 31 for task number 3:{1;3;1}"

    def __init__( self ):
        # Initialize topology
        Topo.__init__( self )

        # Add hosts and switches
        localHost1 = self.addHost ("l1", ip='10.0.0.1')
        localHost2 = self.addHost ("l2", ip='10.0.0.2')
        localHost3 = self.addHost ("l3", ip='10.0.0.3')

        globalHost1 = self.addHost("h1", ip='10.0.0.10')
        globalHost2 = self.addHost("h2", ip='10.0.0.11')
        globalHost3 = self.addHost("h3", ip='10.0.0.12')

        localSwitch = self.addSwitch ("s1", protocols='OpenFlow13')
        natSwitch = self.addSwitch ("s2", protocols='OpenFlow13')
        globalSwitch = self.addSwitch ("s4", protocols='OpenFlow13')
        globalUpperSwitch = self.addSwitch ("sg3", protocols='OpenFlow13')
        globalLowerSwitch = self.addSwitch ("sg5", protocols='OpenFlow13')

        # Add links
        self.addLink (localHost1, localSwitch)
        self.addLink (localHost2, localSwitch)
        self.addLink (localHost3, localSwitch)
        self.addLink (localSwitch, natSwitch)
        self.addLink (natSwitch, globalSwitch)
        self.addLink (globalSwitch, globalUpperSwitch)
        self.addLink (globalSwitch, globalLowerSwitch)
        self.addLink (globalSwitch, globalHost2)
        self.addLink (globalUpperSwitch, globalHost1)
        self.addLink (globalLowerSwitch, globalHost3)

class SimplestTopo(Topo):
    """ Topology for :  balancer1 ---
                                    | switch | --- host1
                        balancer2 ---
    """
    def __init__( self ):
        # Initialize topology
        Topo.__init__( self )

        # Add hosts and switches
        localHost1 = self.addHost ("b1")
        localHost2 = self.addHost ("b2")

        host = self.addHost ("h3")

        natSwitch = self.addSwitch ("s1", protocols='OpenFlow13')

        # Add links
        self.addLink (localHost1, natSwitch)
        self.addLink (localHost2, natSwitch)

        self.addLink (natSwitch, host)

class SimpleTopo(Topo):
    """ Topology for :       nat1----
                                    | switch | -- | switch| --- host1
                             nat2----
    """
    def __init__( self ):
        # Initialize topology
        Topo.__init__( self )

        # Add hosts and switches
        localHost1 = self.addHost ("b1")
        localHost2 = self.addHost ("b2")

        host = self.addHost ("h3")

        natSwitch = self.addSwitch ("s1", protocols='OpenFlow13')
        additionalSwitch = self.addSwitch ("s2", protocols='OpenFlow13')

        # Add links
        self.addLink (localHost1, natSwitch)
        self.addLink (localHost2, natSwitch)

        self.addLink (natSwitch, additionalSwitch)
        self.addLink (additionalSwitch, host)

topos = { 'nat4': ( lambda: NatTopology4() ), 'simplest_topo': ( lambda: SimplestTopo() ),
          'simple_topo': ( lambda: SimpleTopo() ) }
