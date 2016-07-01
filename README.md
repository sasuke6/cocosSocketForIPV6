# cocosSocketForIPV6
The socket framework base on cocos_socket is suitable for IPV6-Only

### How to work on IPV6-Only Environment

1、You need to set two variable

```
bool m_isIpv6
sockaddr_in6 m_serverAddr6
``` 

`m_isIpv6` variable is used for judging the network environment.If the enviroment is IPV6,the function has to adjust the parameter.
`m_serverAddr6` variable is set for IPV6 address.

2、How to make implementation

```
switch (answer->ai_family)
    {
    case AF_UNSPEC: 
        //do something here 
        break; 
    case AF_INET:
        m_isIpv6 = false;

        m_serverAddr4 = *reinterpret_cast<sockaddr_in *>( answer->ai_addr);
        m_serverAddr4.sin_port = htons(serverPort);
        break;
    case AF_INET6: 
        m_isIpv6 = true;

        m_serverAddr6 = *reinterpret_cast<sockaddr_in6 *>( answer->ai_addr);
        m_serverAddr6.sin6_port = htons(serverPort);
        break; 
    } 
```

This is the socket API for the IPV6,and the thing you have to do is that you need to set up parameter right.

```
if(m_isIpv6)
	{
		m_socket = cocos_socket(AF_INET6, SOCK_STREAM, 0);
	}
	else
	{
		m_socket = cocos_socket(AF_INET, SOCK_STREAM, 0);
	}

	if(m_socket == INVALID_SOCKET) return false;

	int rtn = 0;

	if(m_isIpv6)
	{
		rtn = cocos_connect(m_socket, (struct sockaddr *)&m_serverAddr6,
			sizeof(m_serverAddr6));
	}
	else
	{
		rtn = cocos_connect(m_socket, (struct sockaddr *)&m_serverAddr4,
			sizeof(m_serverAddr4));
	}
```
cocos_connect has to change for IPV6,but only the paramenter 
`AF_INET6`.

That is All.

### License

MIT License




