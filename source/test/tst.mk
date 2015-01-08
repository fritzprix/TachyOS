
TEST_PATH=$(ROOT_DIR)/source/test/
TEST_BUILD_BASE_DIR=$(GEN_DIR)/test
GEN_SUB_DIR+=$(TEST_BUILD_BASE_DIR)

$(TEST_BUILD_BASE_DIR):
	$(MK) $(TEST_BUILD_BASE_DIR)
	
include $(TEST_PATH)/sys/sys.mk
include $(TEST_PATH)/hal/haltst.mk



