message(DEBUG "Add Qt test:")
include(LidarGUITesting)
#-----------------------------------------------------------------------------
# Fill this list with tests that perfom screenshots
# Fill this list with tests that does not need baselines
function(set_tests tests_with_baselines tests_without_baselines)
	set(_tests_with_baselines
		NoRPMIfNoData
		OpenHDL64Dual
		OpenHDL64Livestream
		ShowRPMHDL64
		)

	set(_tests_without_baselines
		RulerActionDisabledOnNonOrthogonalView
	  )

	set(${tests_with_baselines} ${_tests_with_baselines} PARENT_SCOPE)
	set(${tests_without_baselines} ${_tests_without_baselines} PARENT_SCOPE)
endfunction()

#-----------------------------------------------------------------------------
# Create lists of tests
set_tests(tests_with_baselines tests_without_baselines)

#-----------------------------------------------------------------------------
# Add here simple UI tests (such as check if this spinbox is fine, etc).
add_GUI_tests_without_baselines(LidarView "${tests_without_baselines}")

#-----------------------------------------------------------------------------
# Add here tests which require a comparison with the render view.
# Keep in mind, you have to manually create the baseline and put it on the right
# folder.
add_GUI_tests_with_baselines(LidarView "${tests_with_baselines}")
