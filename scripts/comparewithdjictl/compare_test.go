package main

import (
	"bufio"
	"bytes"
	"fmt"
	"os/exec"
	"regexp"
	"strings"
	"testing"

	"github.com/sergi/go-diff/diffmatchpatch"
)

func runCommand(name string, args []string, dir string) (string, error) {
	cmd := exec.Command(name, args...)
	if dir != "" {
		cmd.Dir = dir
	}
	var out bytes.Buffer
	cmd.Stdout = &out
	cmd.Stderr = &out
	err := cmd.Run()
	return out.String(), err
}

func parseHex(output string) []string {
	var hexes []string
	re := regexp.MustCompile(`(SENT|RECV)_HEX: ([0-9A-Fa-f]+)`)
	scanner := bufio.NewScanner(strings.NewReader(output))
	for scanner.Scan() {
		line := scanner.Text()
		matches := re.FindStringSubmatch(line)
		if len(matches) == 3 {

			hexes = append(hexes, fmt.Sprintf("%s: %s", matches[1], strings.ToUpper(matches[2])))
		}
	}
	return hexes
}

func TestParity(t *testing.T) {

	t.Log("Running Go test...")
	goOutput, err := runCommand("go", []string{"test", "-v", "./cmd/djictl/...", "-run", "TestConnectWiFiAndStartStreaming"}, "../../../djictl")
	if err != nil {
		t.Logf("Go test output:\n%s", goOutput)
		t.Fatalf("Failed to run Go test: %v", err)
	}

	goHexes := parseHex(goOutput)
	t.Logf("Go Hexes: %v", goHexes)

	t.Log("Running C++ test...")

	var cppOutput string

	possiblePaths := []struct {
		cmd  string
		args []string
		dir  string
	}{
		{"../build/tests/dji_tests", []string{}, "../"},
		{"./tests/dji_tests", []string{}, "../build"},
		{"ctest", []string{"-R", "tst_connect_flow", "-V"}, "../build"},
	}

	found := false
	for _, p := range possiblePaths {
		out, err := runCommand(p.cmd, p.args, p.dir)

		if strings.Contains(out, "SENT_HEX") || strings.Contains(out, "RECV_HEX") {
			cppOutput = out
			found = true
			break
		}

		if err != nil && (strings.Contains(out, "SENT_HEX") || strings.Contains(out, "RECV_HEX")) {
			cppOutput = out
			found = true
			break
		}
	}

	if !found {
		t.Fatalf("Failed to run C++ test or no valid output captured.")
	}

	cppHexes := parseHex(cppOutput)
	t.Logf("C++ Hexes: %v", cppHexes)

	t.Logf("Go Sent Frames: %d", countFrames(goHexes, "SENT"))
	t.Logf("C++ Sent Frames: %d", countFrames(cppHexes, "SENT"))

	goSent := filterFrames(goHexes, "SENT")
	cppSent := filterFrames(cppHexes, "SENT")

	if strings.Join(goSent, "\n") != strings.Join(cppSent, "\n") {
		t.Errorf("Sent frames do not match:\n%s", diffStrings(goSent, cppSent))
	}

	t.Logf("Go Recv Frames: %d", countFrames(goHexes, "RECV"))
	t.Logf("C++ Recv Frames: %d", countFrames(cppHexes, "RECV"))

	goRecv := filterFrames(goHexes, "RECV")
	cppRecv := filterFrames(cppHexes, "RECV")

	if strings.Join(goRecv, "\n") != strings.Join(cppRecv, "\n") {
		t.Errorf("Recv frames do not match:\n%s", diffStrings(goRecv, cppRecv))
	}
}

func countFrames(hexes []string, prefix string) int {
	count := 0
	for _, h := range hexes {
		if strings.HasPrefix(h, prefix) {
			count++
		}
	}
	return count
}

func filterFrames(hexes []string, prefix string) []string {
	var res []string
	for _, h := range hexes {
		if strings.HasPrefix(h, prefix) {
			res = append(res, strings.TrimPrefix(strings.TrimPrefix(h, prefix), ": "))
		}
	}
	return res
}

func diffStrings(a, b []string) string {
	dmp := diffmatchpatch.New()
	diffs := dmp.DiffMain(strings.Join(a, "\n"), strings.Join(b, "\n"), false)
	return dmp.DiffPrettyText(diffs)
}
