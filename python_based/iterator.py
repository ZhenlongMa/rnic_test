import case_driver
import test_config
import analyzer

class iterator:
    def __init__(self, config, driver, analyzer: analyzer.analyzer):
        self.config = config
        self.driver = driver
        self.reach_destination = False
        self.analyzer = analyzer
    
    # set test destination of each iteration
    def set_destination(self):
        a = 0

    def launch_test(self):
        while self.reach_destination() != True:
            self.driver.start_test() # run test and generate test_result_xx files
            self.analyzer.calculate_throughput()
            self.set_next_case()

    def set_next_case(self):
        a = 0

    def reach_destination(self):
        a = 0

    # test peak performance of each kind of instance, and record the result in recorder
    def test_norm(self):
        a = 0