# pylint: disable=unused-import
from abc import ABCMeta, abstractmethod
from pyhxcomm_vx_pybind11 import \
    get_connection_from_env, \
    SimConnection, \
    ARQConnection, \
    Connection


class ConnectionContext(metaclass=ABCMeta):
    """
    Abstract base of context-managers for a hxcomm-Connection handling
    communication with hardware, simulator etc.
    """

    def __init__(self):
        self.connection = None

    @abstractmethod
    def _connect(self):
        """
        Establish a connection by creating
        :py:attr:`ConnectionContext.connection`
        for the respective client (hardware, simulator, ...)
        """
        raise NotImplementedError

    def __enter__(self) -> Connection:
        """
        Create and return the connection object.
        :return Allocated connection object.
        """
        self._connect()
        return self.connection

    def __exit__(self, *args):
        assert self.connection is not None
        del self.connection
        self.connection = None


class AutoContext(ConnectionContext):
    """
    Context-manager for a PlaybackProgramExecutor that connects to hardware
    or simulation based on environment variables.
    """

    def _connect(self):
        self.connection = get_connection_from_env()


class ARQContext(ConnectionContext):
    """
    Context-manager for a PlaybackProgramExecutor with connection to hardware.
    """

    def __init__(self, ip_address: str = None):
        """
        Construct a :py:class:`ARQConnection` based on the FPGA's IP
        address. If none is given, the connection will try to obtain
        the respective value from the environment.

        :param ip_address (optional) IP address of FPGA
        """
        super().__init__()
        self.ip_address = ip_address  # type: str

    def _connect(self):
        if self.ip_address is None:
            self.connection = ARQConnection()
        else:
            self.connection = ARQConnection(self.ip_address)


class SimContext(ConnectionContext):
    """
    Context-manager for a connection to simulator.
    """

    def __init__(self, ip_address: str = None, port: int = None):
        """
        Construct a :py:class:`SimConnection` based on the simulator's IP
        address and port. If neither is given, the connection will try to
        obtain the respective values from the environment.

        :param ip_address (optional) IP address of host running simulation
        :param port (optional) Port for RCF connection to simulation
        """
        super().__init__()
        self.ip_address = ip_address  # type: str
        self.port = port  # type: int

    def _connect(self):
        if self.ip_address is None and self.port is None:
            self.connection = SimConnection()
        else:
            self.connection = SimConnection(self.ip_address, self.port)
