"""
    This is file for topologies for "Computer Networks" task.
    task number 3:{1;3;1}
"""

from mininet.topo import Topo

class Topology31 (Topo):
    "topology number 31 for task number 3:{1;3;1}"

    def __init__( self ):
        # Initialize topology
        Topo.__init__( self )

        # Add hosts and switches
        balancedHost1 = self.addHost ("b1")
        balancedHost2 = self.addHost ("b2")
        balancedHost3 = self.addHost ("b3")

        host1 = self.addHost ("h4")
        host2 = self.addHost ("h5")
        hostUp = self.addHost ("h6")
        hostDown = self.addHost ("h7")

        switchLeft = self.addSwitch ("s1", protocols='OpenFlow13')
        switchRight = self.addSwitch ("s2", protocols='OpenFlow13')
        switchUp = self.addSwitch ("s3", protocols='OpenFlow13')
        switchDown = self.addSwitch ("s4", protocols='OpenFlow13')

        balancerSwitch = self.addSwitch ("s5", protocols='OpenFlow13')

        # Add links
        self.addLink (balancedHost1, balancerSwitch)
        self.addLink (balancedHost2, balancerSwitch)
        self.addLink (balancedHost3, balancerSwitch)

        self.addLink (balancerSwitch, switchLeft)
        self.addLink (switchLeft, switchUp)
        self.addLink (switchLeft, switchDown)
        self.addLink (switchUp, switchRight)
        self.addLink (switchDown, switchRight)

        self.addLink (switchUp, hostUp)
        self.addLink (switchDown, hostDown)
        self.addLink (switchRight, host1)
        self.addLink (switchLeft, host2)

class SimplestTopo(Topo):
    """ Topology for :  balancer1 ---
                                    | switch | --- host1
                        balancer2 ---
    """
    def __init__( self ):
        # Initialize topology
        Topo.__init__( self )

        # Add hosts and switches
        balancedHost1 = self.addHost ("b1")
        balancedHost2 = self.addHost ("b2")

        host = self.addHost ("h3")

        natSwitch = self.addSwitch ("s1", protocols='OpenFlow13')

        # Add links
        self.addLink (balancedHost1, natSwitch)
        self.addLink (balancedHost2, natSwitch)

        self.addLink (natSwitch, host)

class SimpleTopo(Topo):
    """ Topology for :  balancer1 ---
                                    | switch | -- | switch| --- host1
                        balancer2 ---
    """
    def __init__( self ):
        # Initialize topology
        Topo.__init__( self )

        # Add hosts and switches
        balancedHost1 = self.addHost ("b1")
        balancedHost2 = self.addHost ("b2")

        host = self.addHost ("h3")

        natSwitch = self.addSwitch ("s1", protocols='OpenFlow13')
        additionalSwitch = self.addSwitch ("s2", protocols='OpenFlow13')

        # Add links
        self.addLink (balancedHost1, natSwitch)
        self.addLink (balancedHost2, natSwitch)

        self.addLink (natSwitch, additionalSwitch)
        self.addLink (additionalSwitch, host)

class TestTopo (Topo):
    """ Topology for :  host1 --- switch --- host2
                                    |
                        host4 ------ ------- host 4 """

    def __init__( self ):
        # Initialize topology
        Topo.__init__( self )

        # Add hosts and switches
        host1 = self.addHost ("h1")
        host2 = self.addHost ("h2")
        host3 = self.addHost ("h3")
        host4 = self.addHost ("h4")
        switch = self.addSwitch ("s1", protocols='OpenFlow13')

        # Add links
        self.addLink (host1, switch)
        self.addLink (host2, switch)
        self.addLink (host3, switch)
        self.addLink (host4, switch)

topos = { 'topo31': ( lambda: Topology31() ), 'test_topo': ( lambda: TestTopo() ), 'simplest_topo': ( lambda: SimplestTopo() ),
          'simple_topo': ( lambda: SimpleTopo() ) }
