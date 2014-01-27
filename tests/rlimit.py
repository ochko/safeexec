# only root can increase setrlimit

import resource
print(resource.getrlimit(resource.RLIMIT_AS))
resource.setrlimit(resource.RLIMIT_AS, (100000000, 100000000))
print(resource.getrlimit(resource.RLIMIT_AS))
resource.setrlimit(resource.RLIMIT_AS, (100000001, 100000001))
print(resource.getrlimit(resource.RLIMIT_AS))
